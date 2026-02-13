/**************************************************************************/
/*  diff_margin_editor_plugin.cpp                                         */
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

#include "diff_margin_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/editor_interface.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/settings/editor_settings.h"

namespace {
const char *const DIFF_MARGIN_ADDON_PATH = "res://addons/diff-margin";
const char *const DIFF_MARGIN_PLUGIN_NAME = "diff-margin";
const char *const DIFF_MARGIN_SYNC_MARKER_FILE = ".phoenix_sync_revision";
const char *const DIFF_MARGIN_SYNC_REVISION = "2026-02-10-diff-margin-diff-view-guard";
} //namespace

String DiffMarginEditorPlugin::get_plugin_name() const {
	return "Diff Margin";
}

DiffMarginEditorPlugin::DiffMarginEditorPlugin() {
}

DiffMarginEditorPlugin::~DiffMarginEditorPlugin() {
}

void DiffMarginEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_ensure_git_path_setting();
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
		} break;
		case NOTIFICATION_EXIT_TREE: {
			enable_pending = false;
			addon_ready = false;
			set_process(false);
		} break;
	}
}

void DiffMarginEditorPlugin::_ensure_git_path_setting() {
	EditorSettings *settings = EditorSettings::get_singleton();
	if (!settings) {
		return;
	}

	const String setting_key = "plugin/diff-margin/git_path";
	if (!settings->has_setting(setting_key)) {
		settings->set_setting(setting_key, "");
		settings->set_initial_value(setting_key, "", false);
	}

	const String current = settings->get_setting(setting_key);
	if (!current.is_empty()) {
		return;
	}

	const String detected = _find_git_executable();
	if (detected.is_empty()) {
		return;
	}

	settings->set_setting(setting_key, detected);
	settings->set_initial_value(setting_key, detected, false);
	settings->save();
}

String DiffMarginEditorPlugin::_find_git_executable() const {
	Vector<String> candidates;
	String os_name = OS::get_singleton()->get_name();
	const bool is_windows = os_name == "Windows";
	const String git_exe = is_windows ? "git.exe" : "git";

	String path_env = OS::get_singleton()->get_environment("PATH");
	if (!path_env.is_empty()) {
		PackedStringArray path_entries = path_env.split(is_windows ? ";" : ":", false);
		for (int i = 0; i < path_entries.size(); i++) {
			String dir = path_entries[i].strip_edges();
			if (dir.is_empty()) {
				continue;
			}
			candidates.push_back(dir.path_join(git_exe));
		}
	}

	if (is_windows) {
		String program_files = OS::get_singleton()->get_environment("ProgramFiles");
		String program_files_x86 = OS::get_singleton()->get_environment("ProgramFiles(x86)");
		String local_app_data = OS::get_singleton()->get_environment("LocalAppData");

		if (!program_files.is_empty()) {
			candidates.push_back(program_files.path_join("Git").path_join("cmd").path_join(git_exe));
			candidates.push_back(program_files.path_join("Git").path_join("bin").path_join(git_exe));
		}
		if (!program_files_x86.is_empty()) {
			candidates.push_back(program_files_x86.path_join("Git").path_join("cmd").path_join(git_exe));
			candidates.push_back(program_files_x86.path_join("Git").path_join("bin").path_join(git_exe));
		}
		if (!local_app_data.is_empty()) {
			candidates.push_back(local_app_data.path_join("Programs").path_join("Git").path_join("cmd").path_join(git_exe));
			candidates.push_back(local_app_data.path_join("Programs").path_join("Git").path_join("bin").path_join(git_exe));
		}
	} else {
		candidates.push_back("/usr/bin/" + git_exe);
		candidates.push_back("/usr/local/bin/" + git_exe);
		candidates.push_back("/opt/homebrew/bin/" + git_exe);
	}

	for (int i = 0; i < candidates.size(); i++) {
		if (FileAccess::exists(candidates[i])) {
			return candidates[i];
		}
	}

	return "";
}

void DiffMarginEditorPlugin::_maybe_enable_plugin() {
	if (!addon_ready) {
		return;
	}
	EditorInterface *editor_interface = EditorInterface::get_singleton();
	if (!editor_interface) {
		return;
	}
	if (_is_addon_enabled_in_project()) {
		return;
	}
	if (!editor_interface->is_plugin_enabled(DIFF_MARGIN_PLUGIN_NAME)) {
		editor_interface->set_plugin_enabled(DIFF_MARGIN_PLUGIN_NAME, true);
	}
}

bool DiffMarginEditorPlugin::_ensure_addon_installed() {
	const String dst_path = ProjectSettings::get_singleton()->globalize_path(String(DIFF_MARGIN_ADDON_PATH));
	const String dst_plugin_cfg = dst_path.path_join("plugin.cfg");
	const String dst_marker = dst_path.path_join(DIFF_MARGIN_SYNC_MARKER_FILE);
	String marker_revision;
	uint64_t marker_mtime = 0;
	const bool has_marker = _read_sync_marker(dst_marker, marker_revision, marker_mtime);
	const String src_path = _find_source_addon_path();
	if (src_path.is_empty()) {
		ERR_PRINT("Diff Margin addon source not found. Clone the submodule into modules/ultimate_ai/external/godot-diff-margin.");
		return false;
	}
	const uint64_t src_mtime = _get_latest_mtime(src_path);
	const bool marker_valid = has_marker && marker_revision == DIFF_MARGIN_SYNC_REVISION && marker_mtime >= src_mtime;
	if (FileAccess::exists(dst_plugin_cfg) && marker_valid) {
		return true;
	}

	if (DirAccess::dir_exists_absolute(dst_path)) {
		Error clear_err = _remove_dir_contents(dst_path);
		if (clear_err != OK) {
			ERR_PRINT("Diff Margin addon cleanup failed.");
			return false;
		}
	}

	if (_copy_dir_recursive(src_path, dst_path) != OK) {
		ERR_PRINT("Diff Margin addon copy failed.");
		return false;
	}

	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (efs) {
		efs->scan_changes();
	}

	enable_pending = true;
	_write_sync_marker(dst_marker, DIFF_MARGIN_SYNC_REVISION, src_mtime);
	return false;
}

String DiffMarginEditorPlugin::_find_source_addon_path() const {
	String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	Vector<String> candidates;
	candidates.push_back(exec_dir.path_join("modules/ultimate_ai/external/godot-diff-margin/addons/diff-margin").simplify_path());
	candidates.push_back(exec_dir.path_join("../modules/ultimate_ai/external/godot-diff-margin/addons/diff-margin").simplify_path());
	candidates.push_back(exec_dir.path_join("../../modules/ultimate_ai/external/godot-diff-margin/addons/diff-margin").simplify_path());
	candidates.push_back(exec_dir.path_join("../../../modules/ultimate_ai/external/godot-diff-margin/addons/diff-margin").simplify_path());
	candidates.push_back(String(__FILE__).get_base_dir().path_join("../external/godot-diff-margin/addons/diff-margin").simplify_path());

	for (int i = 0; i < candidates.size(); i++) {
		if (DirAccess::dir_exists_absolute(candidates[i])) {
			return candidates[i];
		}
	}

	return "";
}

bool DiffMarginEditorPlugin::_is_addon_enabled_in_project() const {
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (!project_settings || !project_settings->has_setting("editor_plugins/enabled")) {
		return false;
	}
	PackedStringArray enabled = project_settings->get("editor_plugins/enabled");
	const String addon_cfg = String(DIFF_MARGIN_ADDON_PATH).path_join("plugin.cfg");
	return enabled.has(addon_cfg);
}

Error DiffMarginEditorPlugin::_copy_dir_recursive(const String &p_src, const String &p_dst) {
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

uint64_t DiffMarginEditorPlugin::_get_latest_mtime(const String &p_path) const {
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

bool DiffMarginEditorPlugin::_read_sync_marker(const String &p_marker_path, String &r_revision, uint64_t &r_mtime) const {
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

void DiffMarginEditorPlugin::_write_sync_marker(const String &p_marker_path, const String &p_revision, uint64_t p_mtime) const {
	Ref<FileAccess> marker_writer = FileAccess::open(p_marker_path, FileAccess::WRITE);
	if (marker_writer.is_null()) {
		return;
	}
	marker_writer->store_string(String("revision=") + p_revision + "\n");
	marker_writer->store_string(String("mtime=") + String::num_uint64(p_mtime) + "\n");
}

Error DiffMarginEditorPlugin::_remove_dir_contents(const String &p_path) const {
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
