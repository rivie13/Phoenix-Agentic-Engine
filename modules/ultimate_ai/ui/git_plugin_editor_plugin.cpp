/**************************************************************************/
/*  git_plugin_editor_plugin.cpp                                          */
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

#include "git_plugin_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/version_control/editor_vcs_interface.h"
#include "editor/version_control/version_control_editor_plugin.h"

namespace {
const char *const GIT_ADDON_PATH = "res://addons/godot-git-plugin";
const char *const GIT_PLUGIN_CFG = "res://addons/godot-git-plugin/plugin.cfg";
const char *const GIT_SYNC_MARKER_FILE = ".phoenix_sync_revision";
const char *const GIT_SYNC_REVISION = "2026-02-10-godot-git-plugin-integration";
const char *const GIT_VCS_CLASS_NAME = "GitPlugin";
} //namespace

String GitPluginEditorPlugin::get_plugin_name() const {
	return "Godot Git Plugin";
}

GitPluginEditorPlugin::GitPluginEditorPlugin() {
}

void GitPluginEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			bool needs_scan = false;
			if (_ensure_addon_installed(needs_scan)) {
				_enable_addon();
				if (!_maybe_enable_version_control()) {
					vcs_enable_pending = true;
					set_process(true);
				}
			} else if (needs_scan) {
				enable_pending = true;
				set_process(true);
			}
		} break;
		case NOTIFICATION_PROCESS: {
			if (enable_pending) {
				EditorFileSystem *efs = EditorFileSystem::get_singleton();
				if (!efs || !efs->is_scanning()) {
					enable_pending = false;
					_enable_addon();
					if (!_maybe_enable_version_control()) {
						vcs_enable_pending = true;
					}
				}
			}
			if (vcs_enable_pending) {
				if (_maybe_enable_version_control()) {
					vcs_enable_pending = false;
				}
			}
			if (!enable_pending && !vcs_enable_pending) {
				set_process(false);
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			enable_pending = false;
			vcs_enable_pending = false;
			set_process(false);
		} break;
	}
}

void GitPluginEditorPlugin::_enable_addon() {
	EditorNode *editor_node = EditorNode::get_singleton();
	if (!editor_node) {
		return;
	}
	if (_is_addon_enabled_in_project()) {
		return;
	}
	if (editor_node->is_addon_plugin_enabled(GIT_PLUGIN_CFG)) {
		return;
	}
	editor_node->set_addon_plugin_enabled(GIT_PLUGIN_CFG, true, true);
}

bool GitPluginEditorPlugin::_maybe_enable_version_control() {
	if (EditorVCSInterface::get_singleton()) {
		return true;
	}
	if (_is_vcs_autoload_enabled()) {
		return false;
	}
	if (!ClassDB::class_exists(GIT_VCS_CLASS_NAME)) {
		return false;
	}

	VersionControlEditorPlugin *vcs_plugin = VersionControlEditorPlugin::get_singleton();
	if (!vcs_plugin) {
		return false;
	}

	return vcs_plugin->ensure_vcs_plugin_loaded(GIT_VCS_CLASS_NAME, true);
}

bool GitPluginEditorPlugin::_is_addon_enabled_in_project() const {
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (!project_settings || !project_settings->has_setting("editor_plugins/enabled")) {
		return false;
	}
	PackedStringArray enabled = project_settings->get("editor_plugins/enabled");
	return enabled.has(String(GIT_PLUGIN_CFG));
}

bool GitPluginEditorPlugin::_is_vcs_autoload_enabled() const {
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (!project_settings) {
		return false;
	}
	if (!project_settings->get("editor/version_control/autoload_on_startup")) {
		return false;
	}
	String plugin_name = project_settings->get("editor/version_control/plugin_name");
	return plugin_name == GIT_VCS_CLASS_NAME;
}

bool GitPluginEditorPlugin::_ensure_addon_installed(bool &r_needs_scan) {
	r_needs_scan = false;
	const String dst_path = ProjectSettings::get_singleton()->globalize_path(String(GIT_ADDON_PATH));
	const String dst_plugin_cfg = dst_path.path_join("plugin.cfg");
	const String dst_marker = dst_path.path_join(GIT_SYNC_MARKER_FILE);
	String marker_revision;
	uint64_t marker_mtime = 0;
	const bool has_marker = _read_sync_marker(dst_marker, marker_revision, marker_mtime);
	const String src_path = _find_source_addon_path();
	if (src_path.is_empty()) {
		ERR_PRINT("Godot Git Plugin addon source not found. Clone the submodule into modules/ultimate_ai/external/godot-git-plugin.");
		return false;
	}
	const uint64_t src_mtime = _get_latest_mtime(src_path);
	const bool marker_valid = has_marker && marker_revision == GIT_SYNC_REVISION && marker_mtime >= src_mtime;
	if (marker_valid) {
		if (_ensure_plugin_cfg(dst_path) == OK && _ensure_plugin_script(dst_path) == OK && _ensure_windows_binary(dst_path) == OK) {
			return true;
		}
	}

	if (DirAccess::dir_exists_absolute(dst_path)) {
		Error clear_err = _remove_dir_contents(dst_path);
		if (clear_err != OK) {
			ERR_PRINT("Godot Git Plugin addon cleanup failed.");
			return false;
		}
	}

	if (_copy_dir_recursive(src_path, dst_path) != OK) {
		ERR_PRINT("Godot Git Plugin addon copy failed.");
		return false;
	}
	if (_ensure_plugin_cfg(dst_path) != OK) {
		ERR_PRINT("Godot Git Plugin addon plugin.cfg creation failed.");
		return false;
	}
	if (_ensure_plugin_script(dst_path) != OK) {
		ERR_PRINT("Godot Git Plugin addon plugin.gd creation failed.");
		return false;
	}
	if (_ensure_windows_binary(dst_path) != OK) {
		ERR_PRINT("Godot Git Plugin addon Windows binary fixup failed.");
		return false;
	}

	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (efs) {
		efs->scan_changes();
	}

	_write_sync_marker(dst_marker, GIT_SYNC_REVISION, src_mtime);
	r_needs_scan = true;
	return false;
}

Error GitPluginEditorPlugin::_ensure_plugin_cfg(const String &p_dst_path) const {
	const String plugin_cfg = p_dst_path.path_join("plugin.cfg");
	if (FileAccess::exists(plugin_cfg)) {
		Ref<FileAccess> file = FileAccess::open(plugin_cfg, FileAccess::READ);
		if (file.is_null()) {
			return ERR_CANT_OPEN;
		}
		const String contents = file->get_as_text();
		if (contents.find("[plugin]") != -1 && contents.find("script=\"plugin.gd\"") != -1) {
			return OK;
		}
	}

	Ref<FileAccess> file = FileAccess::open(plugin_cfg, FileAccess::WRITE);
	if (file.is_null()) {
		return ERR_CANT_CREATE;
	}

	file->store_string("[plugin]\n");
	file->store_string("\n");
	file->store_string("name=\"Godot Git Plugin\"\n");
	file->store_string("description=\"Git integration for the Godot editor.\"\n");
	file->store_string("author=\"Godot Engine contributors\"\n");
	file->store_string("version=\"1.0\"\n");
	file->store_string("script=\"plugin.gd\"\n");

	return OK;
}

Error GitPluginEditorPlugin::_ensure_plugin_script(const String &p_dst_path) const {
	const String plugin_script = p_dst_path.path_join("plugin.gd");
	if (FileAccess::exists(plugin_script)) {
		Ref<FileAccess> file = FileAccess::open(plugin_script, FileAccess::READ);
		if (file.is_null()) {
			return ERR_CANT_OPEN;
		}
		const String contents = file->get_as_text();
		if (contents.find("extends EditorPlugin") != -1 && contents.find("git_plugin.gdextension") != -1) {
			return OK;
		}
	}

	Ref<FileAccess> file = FileAccess::open(plugin_script, FileAccess::WRITE);
	if (file.is_null()) {
		return ERR_CANT_CREATE;
	}

	file->store_string("@tool\n");
	file->store_string("extends EditorPlugin\n\n");
	file->store_string("var _extension: GDExtension\n\n");
	file->store_string("func _enter_tree() -> void:\n");
	file->store_string("\t_extension = ResourceLoader.load(\"res://addons/godot-git-plugin/git_plugin.gdextension\")\n");
	file->store_string("\tif _extension == null:\n");
	file->store_string("\t\tpush_error(\"Godot Git Plugin: failed to load GDExtension resource.\")\n\n");
	file->store_string("func _exit_tree() -> void:\n");
	file->store_string("\t_extension = null\n");

	return OK;
}

Error GitPluginEditorPlugin::_ensure_windows_binary(const String &p_dst_path) const {
	if (OS::get_singleton()->get_name() != "Windows") {
		return OK;
	}

	const String windows_dir = p_dst_path.path_join("windows");
	const String expected = windows_dir.path_join("libgit_plugin.windows.editor.x86_64.dll");
	if (FileAccess::exists(expected)) {
		return OK;
	}

	const String dev = windows_dir.path_join("libgit_plugin.windows.editor.dev.x86_64.dll");
	if (!FileAccess::exists(dev)) {
		return ERR_FILE_NOT_FOUND;
	}

	Ref<FileAccess> src = FileAccess::open(dev, FileAccess::READ);
	if (src.is_null()) {
		return ERR_CANT_OPEN;
	}
	Ref<FileAccess> dst = FileAccess::open(expected, FileAccess::WRITE);
	if (dst.is_null()) {
		return ERR_CANT_CREATE;
	}

	PackedByteArray buffer = src->get_buffer(src->get_length());
	dst->store_buffer(buffer);
	return OK;
}

String GitPluginEditorPlugin::_find_source_addon_path() const {
	String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	Vector<String> candidates;
	candidates.push_back(exec_dir.path_join("addons/godot-git-plugin").simplify_path());
	candidates.push_back(exec_dir.path_join("../addons/godot-git-plugin").simplify_path());
	candidates.push_back(exec_dir.path_join("modules/ultimate_ai/external/godot-git-plugin/addons/godot-git-plugin").simplify_path());
	candidates.push_back(exec_dir.path_join("../modules/ultimate_ai/external/godot-git-plugin/addons/godot-git-plugin").simplify_path());
	candidates.push_back(exec_dir.path_join("../../modules/ultimate_ai/external/godot-git-plugin/addons/godot-git-plugin").simplify_path());
	candidates.push_back(exec_dir.path_join("../../../modules/ultimate_ai/external/godot-git-plugin/addons/godot-git-plugin").simplify_path());
	candidates.push_back(String(__FILE__).get_base_dir().path_join("../external/godot-git-plugin/addons/godot-git-plugin").simplify_path());

	for (int i = 0; i < candidates.size(); i++) {
		if (DirAccess::dir_exists_absolute(candidates[i])) {
			return candidates[i];
		}
	}

	return "";
}

Error GitPluginEditorPlugin::_copy_dir_recursive(const String &p_src, const String &p_dst) {
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

uint64_t GitPluginEditorPlugin::_get_latest_mtime(const String &p_path) const {
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

bool GitPluginEditorPlugin::_read_sync_marker(const String &p_marker_path, String &r_revision, uint64_t &r_mtime) const {
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

void GitPluginEditorPlugin::_write_sync_marker(const String &p_marker_path, const String &p_revision, uint64_t p_mtime) const {
	Ref<FileAccess> marker_writer = FileAccess::open(p_marker_path, FileAccess::WRITE);
	if (marker_writer.is_null()) {
		return;
	}
	marker_writer->store_string(String("revision=") + p_revision + "\n");
	marker_writer->store_string(String("mtime=") + String::num_uint64(p_mtime) + "\n");
}

Error GitPluginEditorPlugin::_remove_dir_contents(const String &p_path) const {
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
