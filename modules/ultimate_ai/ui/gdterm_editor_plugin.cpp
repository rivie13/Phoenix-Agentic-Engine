/**************************************************************************/
/*  gdterm_editor_plugin.cpp                                              */
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

#include "gdterm_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/editor_interface.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/settings/editor_settings.h"

namespace {
const char *const GDTERM_ADDON_PATH = "res://addons/gdterm";
const char *const GDTERM_PLUGIN_NAME = "gdterm";
const char *const GDTERM_SYNC_MARKER_FILE = ".phoenix_sync_revision";
const char *const GDTERM_SYNC_REVISION = "2026-02-12-gdterm-multi-terminal-panel-v2";
const char *const GDTERM_LAYOUT_SETTING = "gdterm/layout";
const char *const GDTERM_LAYOUT_MIGRATION_SETTING = "gdterm/phoenix_bottom_dock_migrated";
const int GDTERM_LAYOUT_BOTTOM = 1;
} //namespace

String GDTermEditorPlugin::get_plugin_name() const {
	return "GDTerm";
}

GDTermEditorPlugin::GDTermEditorPlugin() {
}

GDTermEditorPlugin::~GDTermEditorPlugin() {
}

void GDTermEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_ensure_bottom_dock_layout_preference();
			_ensure_gitignore_entries();
			set_process(true);
			addon_ready = _ensure_addon_installed();
			_maybe_enable_plugin();
		} break;
		case NOTIFICATION_PROCESS: {
			if (enable_pending) {
				EditorFileSystem *efs = EditorFileSystem::get_singleton();
				if (!efs || !efs->is_scanning()) {
					enable_pending = false;
					addon_ready = _ensure_addon_installed() || addon_ready;
					_maybe_enable_plugin();
				}
			}
			if (!enable_pending) {
				set_process(false);
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			enable_pending = false;
			addon_ready = false;
			layout_migrated_this_run = false;
			set_process(false);
		} break;
	}
}

void GDTermEditorPlugin::_maybe_enable_plugin() {
	if (!addon_ready) {
		return;
	}
	EditorInterface *editor_interface = EditorInterface::get_singleton();
	if (!editor_interface) {
		return;
	}
	const bool addon_enabled_in_project = _is_addon_enabled_in_project();
	bool plugin_enabled = editor_interface->is_plugin_enabled(GDTERM_PLUGIN_NAME);
	if (!addon_enabled_in_project && !plugin_enabled) {
		editor_interface->set_plugin_enabled(GDTERM_PLUGIN_NAME, true);
		plugin_enabled = true;
	}

	if (layout_migrated_this_run && plugin_enabled) {
		editor_interface->set_plugin_enabled(GDTERM_PLUGIN_NAME, false);
		editor_interface->set_plugin_enabled(GDTERM_PLUGIN_NAME, true);
		layout_migrated_this_run = false;
	}
}

void GDTermEditorPlugin::_ensure_bottom_dock_layout_preference() {
	EditorSettings *settings = EditorSettings::get_singleton();
	if (!settings) {
		return;
	}

	bool needs_save = false;
	if (!settings->has_setting(GDTERM_LAYOUT_SETTING)) {
		settings->set_setting(GDTERM_LAYOUT_SETTING, GDTERM_LAYOUT_BOTTOM);
		settings->set_initial_value(GDTERM_LAYOUT_SETTING, GDTERM_LAYOUT_BOTTOM, false);
		needs_save = true;
	}

	if (!settings->has_setting(GDTERM_LAYOUT_MIGRATION_SETTING)) {
		settings->set_setting(GDTERM_LAYOUT_SETTING, GDTERM_LAYOUT_BOTTOM);
		settings->set_initial_value(GDTERM_LAYOUT_SETTING, GDTERM_LAYOUT_BOTTOM, false);
		settings->set_setting(GDTERM_LAYOUT_MIGRATION_SETTING, true);
		settings->set_initial_value(GDTERM_LAYOUT_MIGRATION_SETTING, true, false);
		layout_migrated_this_run = true;
		needs_save = true;
	}

	if (needs_save) {
		settings->save();
	}
}

void GDTermEditorPlugin::_ensure_gitignore_entries() {
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (!project_settings) {
		return;
	}

	const String gitignore_path = project_settings->globalize_path("res://.gitignore");
	const bool has_existing = FileAccess::exists(gitignore_path);
	String gitignore_contents;
	if (has_existing) {
		Ref<FileAccess> file = FileAccess::open(gitignore_path, FileAccess::READ);
		if (file.is_null()) {
			return;
		}
		gitignore_contents = file->get_as_text();
	}

	bool gitignore_changed = false;
	if (!has_existing || gitignore_contents.is_empty()) {
		gitignore_contents = "# Godot 4+ specific ignores\n.godot/\n";
		gitignore_changed = true;
	}

	if (gitignore_contents.find("addons/gdterm/") == -1) {
		if (!gitignore_contents.ends_with("\n")) {
			gitignore_contents += "\n";
		}
		gitignore_contents += "addons/gdterm/\n";
		gitignore_changed = true;
	}

	if (!gitignore_changed) {
		return;
	}

	Ref<FileAccess> out = FileAccess::open(gitignore_path, FileAccess::WRITE);
	if (out.is_null()) {
		return;
	}
	out->store_string(gitignore_contents);
}

bool GDTermEditorPlugin::_ensure_addon_installed() {
	const String dst_path = ProjectSettings::get_singleton()->globalize_path(String(GDTERM_ADDON_PATH));
	const String dst_plugin_cfg = dst_path.path_join("plugin.cfg");
	const String dst_marker = dst_path.path_join(GDTERM_SYNC_MARKER_FILE);
	String marker_revision;
	uint64_t marker_mtime = 0;
	const bool has_marker = _read_sync_marker(dst_marker, marker_revision, marker_mtime);
	const String src_path = _find_source_addon_path();
	if (src_path.is_empty()) {
		ERR_PRINT("GDTerm addon source not found. Clone the submodule into modules/ultimate_ai/external/gdterm.");
		return false;
	}

	const bool dst_exists = FileAccess::exists(dst_plugin_cfg);
	const bool dst_has_binary = _addon_has_extension_binary(dst_path);
	const bool src_has_binary = _addon_has_extension_binary(src_path);
	const uint64_t src_mtime = _get_latest_mtime(src_path);
	const bool marker_valid = has_marker && marker_revision == GDTERM_SYNC_REVISION && marker_mtime >= src_mtime;
	if (dst_exists && marker_valid && dst_has_binary) {
		return true;
	}

	if (!src_has_binary) {
		if (dst_exists && dst_has_binary) {
			return true;
		}
		ERR_PRINT("GDTerm addon binaries are missing. Build GDTerm binaries first.");
		return false;
	}

	if (DirAccess::dir_exists_absolute(dst_path)) {
		Error clear_err = _remove_dir_contents(dst_path);
		if (clear_err != OK) {
			ERR_PRINT("GDTerm addon cleanup failed.");
			return false;
		}
	}

	if (_copy_dir_recursive(src_path, dst_path) != OK) {
		ERR_PRINT("GDTerm addon copy failed.");
		return false;
	}

	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (efs) {
		efs->scan_changes();
	}

	enable_pending = true;
	_write_sync_marker(dst_marker, GDTERM_SYNC_REVISION, src_mtime);
	return false;
}

bool GDTermEditorPlugin::_addon_has_extension_binary(const String &p_addon_path) const {
	const String gdextension_file = p_addon_path.path_join("bin/gdterm.gdextension");
	if (!FileAccess::exists(gdextension_file)) {
		return false;
	}

	const char *const *candidates = nullptr;
	uint32_t candidate_count = 0;

#if defined(LINUXBSD_ENABLED)
	static const char *const linux_candidates[] = {
		"bin/libgdterm.linux.template_debug.x86_64.so",
		"bin/libgdterm.linux.template_release.x86_64.so",
		"bin/libgdterm.linux.template_debug.arm64.so",
		"bin/libgdterm.linux.template_release.arm64.so",
	};
	candidates = linux_candidates;
	candidate_count = sizeof(linux_candidates) / sizeof(linux_candidates[0]);
#elif defined(WINDOWS_ENABLED)
	static const char *const windows_candidates[] = {
		"bin/libgdterm.windows.template_debug.x86_64.dll",
		"bin/libgdterm.windows.template_release.x86_64.dll",
		"bin/libgdterm.windows.template_debug.arm64.dll",
		"bin/libgdterm.windows.template_release.arm64.dll",
	};
	candidates = windows_candidates;
	candidate_count = sizeof(windows_candidates) / sizeof(windows_candidates[0]);
#elif defined(MACOS_ENABLED)
	static const char *const macos_candidates[] = {
		"bin/libgdterm.macos.template_debug.framework",
		"bin/libgdterm.macos.template_release.framework",
	};
	candidates = macos_candidates;
	candidate_count = sizeof(macos_candidates) / sizeof(macos_candidates[0]);
#else
	return true;
#endif

	for (uint32_t i = 0; i < candidate_count; i++) {
		const String library_path = p_addon_path.path_join(candidates[i]);
		if (FileAccess::exists(library_path) || DirAccess::dir_exists_absolute(library_path)) {
			return true;
		}
	}

	return false;
}

String GDTermEditorPlugin::_find_source_addon_path() const {
	String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	Vector<String> candidates;
	candidates.push_back(String(__FILE__).get_base_dir().path_join("../external/gdterm/addons/gdterm").simplify_path());
	candidates.push_back(exec_dir.path_join("modules/ultimate_ai/external/gdterm/addons/gdterm").simplify_path());
	candidates.push_back(exec_dir.path_join("../modules/ultimate_ai/external/gdterm/addons/gdterm").simplify_path());
	candidates.push_back(exec_dir.path_join("../../modules/ultimate_ai/external/gdterm/addons/gdterm").simplify_path());
	candidates.push_back(exec_dir.path_join("../../../modules/ultimate_ai/external/gdterm/addons/gdterm").simplify_path());
	candidates.push_back(exec_dir.path_join("addons/gdterm").simplify_path());
	candidates.push_back(exec_dir.path_join("../addons/gdterm").simplify_path());

	for (int i = 0; i < candidates.size(); i++) {
		if (DirAccess::dir_exists_absolute(candidates[i])) {
			return candidates[i];
		}
	}

	return "";
}

bool GDTermEditorPlugin::_is_addon_enabled_in_project() const {
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (!project_settings || !project_settings->has_setting("editor_plugins/enabled")) {
		return false;
	}
	PackedStringArray enabled = project_settings->get("editor_plugins/enabled");
	const String addon_cfg = String(GDTERM_ADDON_PATH).path_join("plugin.cfg");
	return enabled.has(addon_cfg);
}

Error GDTermEditorPlugin::_copy_dir_recursive(const String &p_src, const String &p_dst) {
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

uint64_t GDTermEditorPlugin::_get_latest_mtime(const String &p_path) const {
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

bool GDTermEditorPlugin::_read_sync_marker(const String &p_marker_path, String &r_revision, uint64_t &r_mtime) const {
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

void GDTermEditorPlugin::_write_sync_marker(const String &p_marker_path, const String &p_revision, uint64_t p_mtime) const {
	Ref<FileAccess> marker_writer = FileAccess::open(p_marker_path, FileAccess::WRITE);
	if (marker_writer.is_null()) {
		return;
	}
	marker_writer->store_string(String("revision=") + p_revision + "\n");
	marker_writer->store_string(String("mtime=") + String::num_uint64(p_mtime) + "\n");
}

Error GDTermEditorPlugin::_remove_dir_contents(const String &p_path) const {
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
