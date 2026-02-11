/**************************************************************************/
/*  pixelpen_editor_plugin.cpp                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "pixelpen_editor_plugin.h"

#include "assistant_panel.h"

#include "core/config/project_settings.h"
#include "core/extension/gdextension_manager.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/translation.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/label.h"
#include "scene/gui/split_container.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"
#include "scene/scene_string_names.h"

namespace {
const char *const PIXELPEN_ADDON_PATH = "res://addons/net.yarvis.pixel_pen";
const char *const PIXELPEN_EXTENSION_PATH = "res://addons/net.yarvis.pixel_pen/pixelpen.gdextension";
const char *const PIXELPEN_WINDOW_SCENE_PATH = "res://addons/net.yarvis.pixel_pen/editor/editor_window.tscn";
const char *const PIXELPEN_MENU_OPEN_WINDOW = "PixelPen: Open Window";
const char *const PIXELPEN_LAYER_LIST_PATH = "Background/VBoxContainer/MarginContainer/Layout/LayerPanel/VBoxContainer/Layers/MarginContainer/ScrollContainer/Layers";
const uint64_t PIXELPEN_CONTEXT_SYNC_INTERVAL_MSEC = 350;
const char *const PIXELPEN_SYNC_MARKER_FILE = ".phoenix_sync_revision";
const char *const PIXELPEN_SYNC_REVISION = "2026-02-10-pixelpen-addon-preload-order-fix";
} //namespace

String PixelPenEditorPlugin::get_plugin_name() const {
	return "PixelPen";
}

bool PixelPenEditorPlugin::has_main_screen() const {
	return true;
}

const Ref<Texture2D> PixelPenEditorPlugin::get_plugin_icon() const {
	return EditorInterface::get_singleton()->get_editor_theme()->get_icon("CanvasItem", "EditorIcons");
}

PixelPenEditorPlugin::PixelPenEditorPlugin() {
}

PixelPenEditorPlugin::~PixelPenEditorPlugin() {
	if (window_instance) {
		window_instance->queue_free();
		window_instance = nullptr;
	}
}

void PixelPenEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			add_tool_menu_item(TTR(PIXELPEN_MENU_OPEN_WINDOW), callable_mp(this, &PixelPenEditorPlugin::_open_window));
			connect(SNAME("main_screen_changed"), callable_mp(this, &PixelPenEditorPlugin::_on_main_screen_changed));
			addon_preload_pending = true;
			set_process(true);
		} break;
		case NOTIFICATION_PROCESS: {
			if (addon_preload_pending && !addon_preload_failed) {
				_preload_addon_if_ready();
			}
			if (open_window_pending) {
				EditorFileSystem *efs = EditorFileSystem::get_singleton();
				if (!efs || !efs->is_scanning()) {
					open_window_pending = false;
					_open_window();
				}
			}
			_sync_context_from_window();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			remove_tool_menu_item(TTR(PIXELPEN_MENU_OPEN_WINDOW));
			if (is_connected(SNAME("main_screen_changed"), callable_mp(this, &PixelPenEditorPlugin::_on_main_screen_changed))) {
				disconnect(SNAME("main_screen_changed"), callable_mp(this, &PixelPenEditorPlugin::_on_main_screen_changed));
			}
			if (window_instance) {
				window_instance->queue_free();
				window_instance = nullptr;
			}
			window_main_ui = nullptr;
			window_assistant_panel = nullptr;
			last_context_snapshot.clear();
			last_context_layers.clear();
			UltimateAssistantPanel::broadcast_pixelpen_context(Dictionary(), Array());
			if (extension_loaded) {
				GDExtensionManager::get_singleton()->unload_extension(PIXELPEN_EXTENSION_PATH);
				extension_loaded = false;
			}
			open_window_pending = false;
			addon_preload_pending = false;
			addon_preload_failed = false;
			preloaded_scripts.clear();
			class_scripts_preloaded = false;
			set_process(false);
		} break;
	}
}

void PixelPenEditorPlugin::make_visible(bool p_visible) {
	if (!p_visible) {
		return;
	}
	_open_window();
	String target_screen = last_main_screen;
	if (target_screen.is_empty() || target_screen == get_plugin_name()) {
		target_screen = "3D";
	}
	EditorInterface::get_singleton()->set_main_screen_editor(target_screen);
}

void PixelPenEditorPlugin::_open_window() {
	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (efs && efs->is_scanning()) {
		open_window_pending = true;
		return;
	}
	open_window_pending = false;

	if (!_ensure_addon_installed(true)) {
		return;
	}

	efs = EditorFileSystem::get_singleton();
	if (efs && efs->is_scanning()) {
		open_window_pending = true;
		return;
	}

	GDExtensionManager::LoadStatus status = GDExtensionManager::get_singleton()->load_extension(PIXELPEN_EXTENSION_PATH);
	if (status != GDExtensionManager::LOAD_STATUS_OK && status != GDExtensionManager::LOAD_STATUS_ALREADY_LOADED) {
		ERR_PRINT("PixelPen extension failed to load. Build PixelPen binaries in modules/ultimate_ai/external/pixelpen first.");
		return;
	}
	extension_loaded = true;
	_preload_class_scripts();
	if (!class_scripts_preloaded) {
		open_window_pending = true;
		return;
	}

	if (window_instance) {
		window_instance->show();
		window_instance->grab_focus();
		return;
	}

	Ref<PackedScene> window_scene = ResourceLoader::load(PIXELPEN_WINDOW_SCENE_PATH);
	if (!window_scene.is_valid()) {
		ERR_PRINT("PixelPen editor window scene is missing.");
		return;
	}

	window_instance = Object::cast_to<Window>(window_scene->instantiate());
	if (!window_instance) {
		ERR_PRINT("PixelPen editor window scene did not instantiate as Window.");
		return;
	}

	window_instance->set("window_running", true);
	window_instance->set_meta("_pixelpen_last_main_screen", last_main_screen);
	EditorNode::get_singleton()->get_gui_base()->add_child(window_instance);
	window_instance->show();
	window_instance->grab_focus();
	window_instance->connect(SceneStringName(tree_exited), callable_mp(this, &PixelPenEditorPlugin::_on_window_exited));

	last_context_sync_msec = 0;
	last_context_snapshot.clear();
	last_context_layers.clear();
}

void PixelPenEditorPlugin::_preload_addon_if_ready() {
	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (efs && efs->is_scanning()) {
		return;
	}
	if (_find_source_addon_path().is_empty()) {
		ERR_PRINT("PixelPen addon source not found. Clone the submodule into modules/ultimate_ai/external/pixelpen.");
		addon_preload_failed = true;
		addon_preload_pending = false;
		return;
	}

	if (!_ensure_addon_installed(false)) {
		if (efs && efs->is_scanning()) {
			return;
		}
		addon_preload_failed = true;
		addon_preload_pending = false;
		return;
	}

	GDExtensionManager::LoadStatus status = GDExtensionManager::get_singleton()->load_extension(PIXELPEN_EXTENSION_PATH);
	if (status != GDExtensionManager::LOAD_STATUS_OK && status != GDExtensionManager::LOAD_STATUS_ALREADY_LOADED) {
		ERR_PRINT("PixelPen extension failed to preload. Build PixelPen binaries in modules/ultimate_ai/external/pixelpen first.");
		addon_preload_failed = true;
		return;
	}
	if (!extension_loaded) {
		extension_loaded = true;
	}

	_preload_class_scripts();
	if (!class_scripts_preloaded) {
		if (efs && efs->is_scanning()) {
			addon_preload_pending = true;
		} else {
			addon_preload_failed = true;
			addon_preload_pending = false;
		}
		return;
	}

	addon_preload_pending = false;
}

void PixelPenEditorPlugin::_ensure_window_layout() {
	if (!window_instance || !window_instance->is_inside_tree()) {
		return;
	}
	if (window_assistant_panel && window_main_ui) {
		return;
	}

	Control *pixelpen_ui = nullptr;
	for (int i = 0; i < window_instance->get_child_count(); i++) {
		Control *child_control = Object::cast_to<Control>(window_instance->get_child(i));
		if (!child_control) {
			continue;
		}
		if (child_control->get_name() == "PhoenixPixelPenSplit") {
			HSplitContainer *split = Object::cast_to<HSplitContainer>(child_control);
			if (!split || split->get_child_count() < 2) {
				return;
			}
			window_main_ui = Object::cast_to<Control>(split->get_child(0));
			window_assistant_panel = Object::cast_to<UltimateAssistantPanel>(split->get_child(1));
			return;
		}
		if (child_control->get_name() == "EditorMainUI") {
			pixelpen_ui = child_control;
			break;
		}
		if (!pixelpen_ui) {
			pixelpen_ui = child_control;
		}
	}
	if (!pixelpen_ui) {
		return;
	}

	HSplitContainer *split = memnew(HSplitContainer);
	split->set_name("PhoenixPixelPenSplit");
	split->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	split->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	window_instance->remove_child(pixelpen_ui);
	window_instance->add_child(split);
	window_instance->move_child(split, 0);
	split->add_child(pixelpen_ui);
	pixelpen_ui->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pixelpen_ui->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	window_assistant_panel = memnew(UltimateAssistantPanel);
	window_assistant_panel->set_name("PixelPenAssistant");
	window_assistant_panel->set_custom_minimum_size(Size2(360, 0));
	window_assistant_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	window_assistant_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	split->add_child(window_assistant_panel);
	split->set_split_offset(980);

	window_main_ui = pixelpen_ui;
}

Dictionary PixelPenEditorPlugin::_build_snapshot() const {
	Dictionary snapshot;
	snapshot["source"] = "pixelpen";
	snapshot["window_open"] = window_instance != nullptr;
	snapshot["last_main_screen"] = last_main_screen;
	if (!window_instance) {
		return snapshot;
	}

	String title = window_instance->get_title();
	snapshot["window_title"] = title;
	snapshot["focused"] = window_instance->has_focus();

	String clean_title = title;
	if (clean_title.begins_with("(*)")) {
		clean_title = clean_title.substr(3).strip_edges();
	}
	int separator_idx = clean_title.find(" - ");
	if (separator_idx > 0) {
		String left = clean_title.substr(0, separator_idx).strip_edges();
		int canvas_idx = left.find(" (");
		if (canvas_idx > 0 && left.ends_with(")")) {
			snapshot["project_name"] = left.substr(0, canvas_idx).strip_edges();
			snapshot["canvas_info"] = left.substr(canvas_idx + 2, left.length() - canvas_idx - 3);
		} else {
			snapshot["project_name"] = left;
		}
	}
	return snapshot;
}

Array PixelPenEditorPlugin::_collect_layer_snapshot() const {
	Array layers;
	if (!window_main_ui) {
		return layers;
	}
	Node *layer_list = window_main_ui->get_node_or_null(NodePath(PIXELPEN_LAYER_LIST_PATH));
	if (!layer_list) {
		return layers;
	}

	for (int i = 0; i < layer_list->get_child_count(); i++) {
		Control *layer_row = Object::cast_to<Control>(layer_list->get_child(i));
		if (!layer_row || layer_row->get_name() == "SeparatorHint") {
			continue;
		}

		String label_text = layer_row->get_name();
		Variant label_variant = layer_row->get("label");
		if (label_variant.get_type() == Variant::OBJECT) {
			Object *label_obj = label_variant;
			Label *label = Object::cast_to<Label>(label_obj);
			if (label && !label->get_text().is_empty()) {
				label_text = label->get_text();
			}
		}

		bool layer_visible = true;
		Variant visible_variant = layer_row->get("layer_visible");
		if (visible_variant.get_type() == Variant::BOOL) {
			layer_visible = visible_variant;
		}

		bool active = false;
		Variant active_rect_variant = layer_row->get("active_rect");
		Variant active_color_variant = layer_row->get("active_color");
		if (active_rect_variant.get_type() == Variant::OBJECT && active_color_variant.get_type() == Variant::COLOR) {
			Object *active_rect_obj = active_rect_variant;
			ColorRect *active_rect = Object::cast_to<ColorRect>(active_rect_obj);
			if (active_rect) {
				Color active_color = active_color_variant;
				active = active_rect->get_self_modulate().is_equal_approx(active_color);
			}
		}

		Dictionary row;
		row["label"] = label_text;
		row["uid"] = layer_row->get("layer_uid");
		row["visible"] = layer_visible;
		row["active"] = active;
		layers.push_back(row);
	}

	return layers;
}

void PixelPenEditorPlugin::_sync_context_from_window() {
	if (!window_instance || !window_instance->is_inside_tree()) {
		return;
	}
	_ensure_window_layout();

	const uint64_t now_msec = Time::get_singleton()->get_ticks_msec();
	if (now_msec - last_context_sync_msec < PIXELPEN_CONTEXT_SYNC_INTERVAL_MSEC) {
		return;
	}
	last_context_sync_msec = now_msec;

	Array layers = _collect_layer_snapshot();
	Dictionary snapshot = _build_snapshot();
	snapshot["layer_count"] = layers.size();

	if (snapshot == last_context_snapshot && layers == last_context_layers) {
		return;
	}

	last_context_snapshot = snapshot;
	last_context_layers = layers;
	UltimateAssistantPanel::broadcast_pixelpen_context(snapshot, layers);
}

bool PixelPenEditorPlugin::_preload_script(const char *p_path) {
	Ref<Resource> res = ResourceLoader::load(p_path);
	if (res.is_valid()) {
		preloaded_scripts.push_back(res);
		return true;
	}
	ERR_PRINT(vformat("PixelPen class preload failed: %s", p_path));
	return false;
}

void PixelPenEditorPlugin::_preload_class_scripts() {
	if (class_scripts_preloaded) {
		return;
	}
	preloaded_scripts.clear();

	const char *const scripts[] = {
		"res://addons/net.yarvis.pixel_pen/classes/pixelpen_enum.gd",
		"res://addons/net.yarvis.pixel_pen/classes/editor_shorcut.gd",
		"res://addons/net.yarvis.pixel_pen/classes/mask_selection.gd",
		"res://addons/net.yarvis.pixel_pen/classes/undo_redo_manager.gd",
		"res://addons/net.yarvis.pixel_pen/classes/indexed_palette.gd",
		"res://addons/net.yarvis.pixel_pen/classes/indexed_color_image.gd",
		"res://addons/net.yarvis.pixel_pen/classes/frame.gd",
		"res://addons/net.yarvis.pixel_pen/classes/animation_cell.gd",
		"res://addons/net.yarvis.pixel_pen/classes/pixel_pen_project.gd",
		"res://addons/net.yarvis.pixel_pen/classes/project_packer.gd",
		"res://addons/net.yarvis.pixel_pen/classes/user_config.gd",
		"res://addons/net.yarvis.pixel_pen/classes/pixelpen_state.gd",
		"res://addons/net.yarvis.pixel_pen/classes/pixelpen.gd",
		"res://addons/net.yarvis.pixel_pen/classes/theme_config.gd",
		"res://addons/net.yarvis.pixel_pen/ui/layout_split/branch.gd",
		"res://addons/net.yarvis.pixel_pen/ui/layout_split/data_branch.gd",
		"res://addons/net.yarvis.pixel_pen/ui/layout_split/layout_split.gd",
		"res://addons/net.yarvis.pixel_pen/ui/tree_properties/tree_row.gd",
		"res://addons/net.yarvis.pixel_pen/ui/tree_properties/toggle_button.gd",
		"res://addons/net.yarvis.pixel_pen/ui/tree_properties/tree_properties.gd",
	};

	bool all_loaded = true;
	for (uint32_t i = 0; i < sizeof(scripts) / sizeof(scripts[0]); i++) {
		all_loaded = _preload_script(scripts[i]) && all_loaded;
	}

	class_scripts_preloaded = all_loaded;
	if (!class_scripts_preloaded) {
		preloaded_scripts.clear();
		ERR_PRINT("PixelPen class preload incomplete, will retry once filesystem scanning finishes.");
	}
}

void PixelPenEditorPlugin::_on_window_exited() {
	window_instance = nullptr;
	window_main_ui = nullptr;
	window_assistant_panel = nullptr;
	last_context_snapshot.clear();
	last_context_layers.clear();
	UltimateAssistantPanel::broadcast_pixelpen_context(Dictionary(), Array());
}

void PixelPenEditorPlugin::_on_main_screen_changed(const String &p_screen_name) {
	if (p_screen_name == get_plugin_name()) {
		return;
	}
	last_main_screen = p_screen_name;
	if (window_instance) {
		window_instance->set_meta("_pixelpen_last_main_screen", last_main_screen);
	}
}

bool PixelPenEditorPlugin::_ensure_addon_installed(bool p_allow_open) {
	const String dst_path = ProjectSettings::get_singleton()->globalize_path(String(PIXELPEN_ADDON_PATH));
	const String dst_plugin_cfg = dst_path.path_join("plugin.cfg");
	const String dst_marker = dst_path.path_join(PIXELPEN_SYNC_MARKER_FILE);
	String marker_revision;
	uint64_t marker_mtime = 0;
	bool did_copy = false;
	const bool has_marker = _read_sync_marker(dst_marker, marker_revision, marker_mtime);
	const String src_path = _find_source_addon_path();
	if (src_path.is_empty()) {
		ERR_PRINT("PixelPen addon source not found. Clone the submodule into modules/ultimate_ai/external/pixelpen.");
		return false;
	}

	const bool dst_exists = FileAccess::exists(dst_plugin_cfg);
	const bool dst_has_binary = _addon_has_extension_binary(dst_path);
	const bool src_has_binary = _addon_has_extension_binary(src_path);

	const uint64_t src_mtime = _get_latest_mtime(src_path);
	const bool marker_valid = has_marker && marker_revision == PIXELPEN_SYNC_REVISION && marker_mtime >= src_mtime;
	if (dst_exists && marker_valid && dst_has_binary) {
		return true;
	}

	// If source binaries are unavailable, keep a previously installed working addon if present.
	if (!src_has_binary) {
		if (dst_exists && dst_has_binary) {
			return true;
		}
		ERR_PRINT("PixelPen addon binaries are missing. Build PixelPen binaries in modules/ultimate_ai/external/pixelpen first.");
		return false;
	}

	if (DirAccess::dir_exists_absolute(dst_path)) {
		Error clear_err = _remove_dir_contents(dst_path);
		if (clear_err != OK) {
			ERR_PRINT("PixelPen addon cleanup failed.");
			return false;
		}
	}

	if (_copy_dir_recursive(src_path, dst_path) != OK) {
		ERR_PRINT("PixelPen addon copy failed.");
		return false;
	}
	did_copy = true;

	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (efs) {
		efs->scan_changes();
	}

	preloaded_scripts.clear();
	class_scripts_preloaded = false;

	_write_sync_marker(dst_marker, PIXELPEN_SYNC_REVISION, src_mtime);

	if (did_copy) {
		if (p_allow_open) {
			open_window_pending = true;
		}
		return false;
	}

	return FileAccess::exists(dst_plugin_cfg);
}

bool PixelPenEditorPlugin::_addon_has_extension_binary(const String &p_addon_path) const {
#if defined(LINUXBSD_ENABLED) || defined(WINDOWS_ENABLED) || defined(MACOS_ENABLED)
#if defined(LINUXBSD_ENABLED)
	const char *const candidates[] = {
		"bin/libpixelpen.linux.debug.x86_64.so",
		"bin/libpixelpen.linux.release.x86_64.so",
	};
#elif defined(WINDOWS_ENABLED)
	const char *const candidates[] = {
		"bin/libpixelpen.windows.debug.x86_64.dll",
		"bin/libpixelpen.windows.release.x86_64.dll",
		"bin/libpixelpen.windows.debug.x86_32.dll",
		"bin/libpixelpen.windows.release.x86_32.dll",
	};
#elif defined(MACOS_ENABLED)
	const char *const candidates[] = {
		"bin/libpixelpen.macos.debug.framework",
		"bin/libpixelpen.macos.release.framework",
	};
#endif

	for (uint32_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
		const String library_path = p_addon_path.path_join(candidates[i]);
		if (FileAccess::exists(library_path) || DirAccess::dir_exists_absolute(library_path)) {
			return true;
		}
	}

	return false;
#else
	// On other platforms (e.g. Android/iOS/Web), we don't currently ship PixelPen editor
	// extension binaries, so treat it as "available" to avoid blocking addon sync.
	return true;
#endif
}

String PixelPenEditorPlugin::_find_source_addon_path() const {
	String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	Vector<String> candidates;
	candidates.push_back(exec_dir.path_join("modules/ultimate_ai/external/pixelpen/project/addons/net.yarvis.pixel_pen").simplify_path());
	candidates.push_back(exec_dir.path_join("../modules/ultimate_ai/external/pixelpen/project/addons/net.yarvis.pixel_pen").simplify_path());
	candidates.push_back(exec_dir.path_join("../../modules/ultimate_ai/external/pixelpen/project/addons/net.yarvis.pixel_pen").simplify_path());
	candidates.push_back(exec_dir.path_join("../../../modules/ultimate_ai/external/pixelpen/project/addons/net.yarvis.pixel_pen").simplify_path());
	candidates.push_back(String(__FILE__).get_base_dir().path_join("../external/pixelpen/project/addons/net.yarvis.pixel_pen").simplify_path());

	for (int i = 0; i < candidates.size(); i++) {
		if (DirAccess::dir_exists_absolute(candidates[i])) {
			return candidates[i];
		}
	}

	return "";
}

Error PixelPenEditorPlugin::_copy_dir_recursive(const String &p_src, const String &p_dst) {
	Ref<DirAccess> src_dir = DirAccess::open(p_src);
	if (src_dir.is_null()) {
		return ERR_CANT_OPEN;
	}

	Error mk_err = DirAccess::make_dir_recursive_absolute(p_dst);
	if (mk_err != OK) {
		return mk_err;
	}

	src_dir->list_dir_begin();
	while (true) {
		String file_name = src_dir->get_next();
		if (file_name.is_empty()) {
			break;
		}
		if (file_name == "." || file_name == "..") {
			continue;
		}

		String src_path = p_src.path_join(file_name);
		String dst_path = p_dst.path_join(file_name);

		if (src_dir->current_is_dir()) {
			Error err = _copy_dir_recursive(src_path, dst_path);
			if (err != OK) {
				src_dir->list_dir_end();
				return err;
			}
		} else {
			Ref<FileAccess> src_file = FileAccess::open(src_path, FileAccess::READ);
			if (src_file.is_null()) {
				src_dir->list_dir_end();
				return ERR_CANT_OPEN;
			}
			Ref<FileAccess> dst_file = FileAccess::open(dst_path, FileAccess::WRITE);
			if (dst_file.is_null()) {
				src_dir->list_dir_end();
				return ERR_CANT_CREATE;
			}
			PackedByteArray buffer = src_file->get_buffer(src_file->get_length());
			dst_file->store_buffer(buffer);
		}
	}

	src_dir->list_dir_end();
	return OK;
}

uint64_t PixelPenEditorPlugin::_get_latest_mtime(const String &p_path) const {
	Ref<DirAccess> dir = DirAccess::open(p_path);
	if (dir.is_null()) {
		return 0;
	}

	uint64_t latest = 0;
	dir->list_dir_begin();
	while (true) {
		String name = dir->get_next();
		if (name.is_empty()) {
			break;
		}
		if (name == "." || name == "..") {
			continue;
		}
		String path = p_path.path_join(name);
		if (dir->current_is_dir()) {
			uint64_t child_latest = _get_latest_mtime(path);
			if (child_latest > latest) {
				latest = child_latest;
			}
		} else {
			uint64_t mtime = FileAccess::get_modified_time(path);
			if (mtime > latest) {
				latest = mtime;
			}
		}
	}
	dir->list_dir_end();
	return latest;
}

bool PixelPenEditorPlugin::_read_sync_marker(const String &p_marker_path, String &r_revision, uint64_t &r_mtime) const {
	r_revision = "";
	r_mtime = 0;
	if (!FileAccess::exists(p_marker_path)) {
		return false;
	}
	Ref<FileAccess> marker_reader = FileAccess::open(p_marker_path, FileAccess::READ);
	if (marker_reader.is_null()) {
		return false;
	}

	while (!marker_reader->eof_reached()) {
		String line = marker_reader->get_line().strip_edges();
		if (line.begins_with("revision=")) {
			r_revision = line.substr(String("revision=").length()).strip_edges();
		} else if (line.begins_with("mtime=")) {
			String value = line.substr(String("mtime=").length()).strip_edges();
			r_mtime = static_cast<uint64_t>(value.to_int());
		}
	}

	return !r_revision.is_empty();
}

void PixelPenEditorPlugin::_write_sync_marker(const String &p_marker_path, const String &p_revision, uint64_t p_mtime) const {
	Ref<FileAccess> marker_writer = FileAccess::open(p_marker_path, FileAccess::WRITE);
	if (marker_writer.is_null()) {
		return;
	}
	marker_writer->store_string(String("revision=") + p_revision + "\n");
	marker_writer->store_string(String("mtime=") + String::num_uint64(p_mtime) + "\n");
}

Error PixelPenEditorPlugin::_remove_dir_contents(const String &p_path) const {
	Ref<DirAccess> dir = DirAccess::open(p_path);
	if (dir.is_null()) {
		return ERR_CANT_OPEN;
	}

	dir->list_dir_begin();
	while (true) {
		String name = dir->get_next();
		if (name.is_empty()) {
			break;
		}
		if (name == "." || name == "..") {
			continue;
		}
		String path = p_path.path_join(name);
		if (dir->current_is_dir()) {
			Error err = _remove_dir_contents(path);
			if (err != OK) {
				dir->list_dir_end();
				return err;
			}
			DirAccess::remove_absolute(path);
		} else {
			DirAccess::remove_absolute(path);
		}
	}
	dir->list_dir_end();
	return OK;
}
