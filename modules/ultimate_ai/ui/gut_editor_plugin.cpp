/**************************************************************************/
/*  gut_editor_plugin.cpp                                                 */
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

#include "gut_editor_plugin.h"

#include "addon_bootstrap_utils.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"

namespace {
const char *const GUT_ADDON_PATH = "res://addons/gut";
const char *const GUT_PLUGIN_CFG = "res://addons/gut/plugin.cfg";
const char *const GUT_SYNC_MARKER_FILE = ".phoenix_sync_revision";
const char *const GUT_SYNC_REVISION = "2026-02-16-gut-addon-bootstrap-v1";
} //namespace

String GutEditorPlugin::get_plugin_name() const {
	return "GUT";
}

GutEditorPlugin::GutEditorPlugin() {
}

GutEditorPlugin::~GutEditorPlugin() {
}

void GutEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			AddonBootstrapMigrator::ensure_default_gitignore_entries_once();
			_ensure_gitignore_entries();
			addon_ready = _ensure_addon_installed();
			addon_activation_pending = addon_ready;
			addon_enable_delay_frames = addon_ready ? 1 : 0;
			set_process(enable_pending || addon_activation_pending);
		} break;
		case NOTIFICATION_PROCESS: {
			if (enable_pending) {
				EditorFileSystem *efs = EditorFileSystem::get_singleton();
				if (efs && efs->is_scanning()) {
					return;
				}

				enable_pending = false;
				addon_ready = _ensure_addon_installed() || addon_ready;
				if (addon_ready) {
					addon_activation_pending = true;
					addon_enable_delay_frames = 1;
				}
			}

			if (addon_activation_pending && addon_ready) {
				if (addon_enable_delay_frames > 0) {
					addon_enable_delay_frames--;
					return;
				}

				if (_maybe_enable_plugin()) {
					addon_activation_pending = false;
				}
			}

			if (!enable_pending && !addon_activation_pending) {
				set_process(false);
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			enable_pending = false;
			addon_ready = false;
			addon_activation_pending = false;
			addon_enable_delay_frames = 0;
			set_process(false);
		} break;
	}
}

bool GutEditorPlugin::_maybe_enable_plugin() {
	if (!addon_ready) {
		return true;
	}

	if (!FileAccess::exists(GUT_PLUGIN_CFG)) {
		return true;
	}

	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (!project_settings || !project_settings->has_setting("editor_plugins/enabled")) {
		return false;
	}

	EditorNode *editor_node = EditorNode::get_singleton();
	if (!editor_node) {
		return false;
	}

	if (_is_addon_enabled_in_project()) {
		return true;
	}

	if (editor_node->is_addon_plugin_enabled(GUT_PLUGIN_CFG)) {
		return true;
	}

	editor_node->set_addon_plugin_enabled(GUT_PLUGIN_CFG, true, true);
	return true;
}

void GutEditorPlugin::_ensure_gitignore_entries() {
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

	if (gitignore_contents.find("addons/gut/") == -1) {
		if (!gitignore_contents.ends_with("\n")) {
			gitignore_contents += "\n";
		}
		gitignore_contents += "addons/gut/\n";
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

bool GutEditorPlugin::_ensure_addon_installed() {
	const String dst_path = ProjectSettings::get_singleton()->globalize_path(String(GUT_ADDON_PATH));
	const String dst_plugin_cfg = dst_path.path_join("plugin.cfg");
	const String dst_marker = dst_path.path_join(GUT_SYNC_MARKER_FILE);

	String marker_revision;
	uint64_t marker_mtime = 0;
	const bool has_marker = _read_sync_marker(dst_marker, marker_revision, marker_mtime);

	const String src_path = _find_source_addon_path();
	if (src_path.is_empty()) {
		ERR_PRINT("GUT addon source not found. Clone submodule into modules/ultimate_ai/external/gut.");
		return false;
	}

	const uint64_t src_mtime = _get_latest_mtime(src_path);
	const bool marker_valid = has_marker && marker_revision == GUT_SYNC_REVISION && marker_mtime >= src_mtime;
	if (FileAccess::exists(dst_plugin_cfg) && marker_valid) {
		return true;
	}

	if (DirAccess::dir_exists_absolute(dst_path)) {
		Error clear_err = _remove_dir_contents(dst_path);
		if (clear_err != OK) {
			ERR_PRINT("GUT addon cleanup failed.");
			return false;
		}
	}

	if (_copy_dir_recursive(src_path, dst_path) != OK) {
		ERR_PRINT("GUT addon copy failed.");
		return false;
	}

	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (efs) {
		efs->scan_changes();
	}

	enable_pending = true;
	_write_sync_marker(dst_marker, GUT_SYNC_REVISION, src_mtime);
	return false;
}

String GutEditorPlugin::_find_source_addon_path() const {
	String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	Vector<String> candidates;

	candidates.push_back(exec_dir.path_join("addons/gut").simplify_path());
	candidates.push_back(exec_dir.path_join("../addons/gut").simplify_path());
	candidates.push_back(exec_dir.path_join("modules/ultimate_ai/external/gut/addons/gut").simplify_path());
	candidates.push_back(exec_dir.path_join("../modules/ultimate_ai/external/gut/addons/gut").simplify_path());
	candidates.push_back(exec_dir.path_join("../../modules/ultimate_ai/external/gut/addons/gut").simplify_path());
	candidates.push_back(exec_dir.path_join("../../../modules/ultimate_ai/external/gut/addons/gut").simplify_path());
	candidates.push_back(String(__FILE__).get_base_dir().path_join("../external/gut/addons/gut").simplify_path());

	for (int i = 0; i < candidates.size(); i++) {
		if (DirAccess::dir_exists_absolute(candidates[i])) {
			return candidates[i];
		}
	}

	return "";
}

bool GutEditorPlugin::_is_addon_enabled_in_project() const {
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (!project_settings || !project_settings->has_setting("editor_plugins/enabled")) {
		return false;
	}

	PackedStringArray enabled = project_settings->get("editor_plugins/enabled");
	return enabled.has(String(GUT_ADDON_PATH).path_join("plugin.cfg"));
}

Error GutEditorPlugin::_copy_dir_recursive(const String &p_src, const String &p_dst) {
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

uint64_t GutEditorPlugin::_get_latest_mtime(const String &p_path) const {
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

bool GutEditorPlugin::_read_sync_marker(const String &p_marker_path, String &r_revision, uint64_t &r_mtime) const {
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

void GutEditorPlugin::_write_sync_marker(const String &p_marker_path, const String &p_revision, uint64_t p_mtime) const {
	Ref<FileAccess> marker_writer = FileAccess::open(p_marker_path, FileAccess::WRITE);
	if (marker_writer.is_null()) {
		return;
	}

	marker_writer->store_string(String("revision=") + p_revision + "\n");
	marker_writer->store_string(String("mtime=") + String::num_uint64(p_mtime) + "\n");
}

Error GutEditorPlugin::_remove_dir_contents(const String &p_path) const {
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
