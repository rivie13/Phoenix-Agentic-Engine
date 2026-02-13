/**************************************************************************/
/*  git_plugin_editor_plugin.h                                            */
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

#pragma once

#include "editor/plugins/editor_plugin.h"

class GitPluginEditorPlugin : public EditorPlugin {
	GDCLASS(GitPluginEditorPlugin, EditorPlugin);

	bool enable_pending = false;
	bool vcs_enable_pending = false;

	void _notification(int p_what);

	bool _ensure_addon_installed(bool &r_needs_scan);
	String _find_source_addon_path() const;
	Error _copy_dir_recursive(const String &p_src, const String &p_dst);
	Error _remove_dir_contents(const String &p_path) const;
	Error _ensure_plugin_cfg(const String &p_dst_path) const;
	Error _ensure_plugin_script(const String &p_dst_path) const;
	Error _ensure_windows_binary(const String &p_dst_path) const;
	uint64_t _get_latest_mtime(const String &p_path) const;
	bool _read_sync_marker(const String &p_marker_path, String &r_revision, uint64_t &r_mtime) const;
	void _write_sync_marker(const String &p_marker_path, const String &p_revision, uint64_t p_mtime) const;
	bool _is_addon_enabled_in_project() const;
	bool _is_vcs_autoload_enabled() const;
	void _enable_addon();
	bool _maybe_enable_version_control();

public:
	String get_plugin_name() const override;

	GitPluginEditorPlugin();
};
