/**************************************************************************/
/*  ultimate_ai_editor_plugin.cpp                                         */
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

#include "ultimate_ai_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"

static Error copy_dir_recursive(const String &p_src, const String &p_dst) {
	ERR_FAIL_COND_V(p_src.is_empty() || p_dst.is_empty(), ERR_INVALID_PARAMETER);

	if (!DirAccess::dir_exists_absolute(p_src)) {
		return ERR_DOES_NOT_EXIST;
	}

	DirAccess::make_dir_recursive_absolute(p_dst);

	Ref<DirAccess> src_dir = DirAccess::open(p_src);
	ERR_FAIL_COND_V(src_dir.is_null(), ERR_CANT_OPEN);

	src_dir->list_dir_begin();
	String file_name = src_dir->get_next();
	while (!file_name.is_empty()) {
		if (file_name == "." || file_name == "..") {
			file_name = src_dir->get_next();
			continue;
		}

		String src_path = p_src.path_join(file_name);
		String dst_path = p_dst.path_join(file_name);

		if (src_dir->current_is_dir()) {
			Error dir_err = copy_dir_recursive(src_path, dst_path);
			if (dir_err != OK) {
				src_dir->list_dir_end();
				return dir_err;
			}
		} else {
			Error read_err = OK;
			Vector<uint8_t> data = FileAccess::get_file_as_bytes(src_path, &read_err);
			if (read_err != OK) {
				src_dir->list_dir_end();
				return read_err;
			}
			Ref<FileAccess> dst_file = FileAccess::open(dst_path, FileAccess::WRITE);
			if (dst_file.is_null()) {
				src_dir->list_dir_end();
				return FileAccess::get_open_error();
			}
			dst_file->store_buffer(data.ptr(), data.size());
		}

		file_name = src_dir->get_next();
	}
	src_dir->list_dir_end();

	return OK;
}

static String read_text_file(const String &p_path) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return "";
	}
	return f->get_as_text();
}

static Error ensure_addon_synced() {
	String exe_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	String repo_root = exe_dir.get_base_dir();
	String src_path = repo_root.path_join("modules/ultimate_ai/external/godot-ai-autonomous-agent/addons/ai_autonomous_agent");

	String project_root = ProjectSettings::get_singleton()->get_resource_path();
	String dst_path = project_root.path_join("addons/ai_autonomous_agent");
	Error copy_err = copy_dir_recursive(src_path, dst_path);
	if (copy_err == OK && EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->scan_changes();
	}

	return copy_err;
}

String UltimateAIEditorPlugin::get_plugin_name() const {
	return "Ultimate AI";
}

UltimateAIEditorPlugin::UltimateAIEditorPlugin() {
}

void UltimateAIEditorPlugin::_setup_assistant() {
	String project_root = ProjectSettings::get_singleton()->get_resource_path();
	if (project_root.is_empty()) {
		call_deferred("_setup_assistant");
		return;
	}

	String addon_root = project_root.path_join("addons/ai_autonomous_agent");
	if (!DirAccess::dir_exists_absolute(addon_root)) {
		Error sync_err = ensure_addon_synced();
		if (sync_err != OK) {
			ERR_PRINT("Ultimate AI: Failed to sync ai_autonomous_agent addon into project.");
			return;
		}
		call_deferred("_setup_assistant");
		return;
	}

	EditorNode *editor = EditorNode::get_singleton();
	if (!editor || !editor->is_editor_ready()) {
		call_deferred("_setup_assistant");
		return;
	}

	const String addon_cfg = "res://addons/ai_autonomous_agent/plugin.cfg";
	if (editor->is_addon_plugin_enabled(addon_cfg)) {
		return;
	}

	ProjectSettings *ps = ProjectSettings::get_singleton();
	if (ps->has_setting("editor_plugins/enabled")) {
		PackedStringArray enabled = ps->get("editor_plugins/enabled");
		if (enabled.has(addon_cfg)) {
			return;
		}
	}

	editor->set_addon_plugin_enabled(addon_cfg, true, true);
}

void UltimateAIEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			_setup_assistant();
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
		} break;
	}
}
