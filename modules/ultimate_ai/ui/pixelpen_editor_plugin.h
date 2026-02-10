/**************************************************************************/
/*  pixelpen_editor_plugin.h                                              */
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

class Control;
class Resource;
class UltimateAssistantPanel;
class Window;

class PixelPenEditorPlugin : public EditorPlugin {
	GDCLASS(PixelPenEditorPlugin, EditorPlugin);

	Window *window_instance = nullptr;
	Control *window_main_ui = nullptr;
	UltimateAssistantPanel *window_assistant_panel = nullptr;
	bool class_scripts_preloaded = false;
	bool extension_loaded = false;
	String last_main_screen = "3D";
	Vector<Ref<Resource>> preloaded_scripts;
	uint64_t last_context_sync_msec = 0;
	Dictionary last_context_snapshot;
	Array last_context_layers;

	void _notification(int p_what);

	bool _ensure_addon_installed();
	String _find_source_addon_path() const;
	Error _copy_dir_recursive(const String &p_src, const String &p_dst);
	void _preload_script(const char *p_path);
	void _preload_class_scripts();
	void _open_window();
	void _ensure_window_layout();
	void _sync_context_from_window();
	Array _collect_layer_snapshot() const;
	Dictionary _build_snapshot() const;
	uint64_t _get_latest_mtime(const String &p_path) const;
	bool _read_sync_marker(const String &p_marker_path, String &r_revision, uint64_t &r_mtime) const;
	void _write_sync_marker(const String &p_marker_path, const String &p_revision, uint64_t p_mtime) const;
	Error _remove_dir_contents(const String &p_path) const;
	void _on_window_exited();
	void _on_main_screen_changed(const String &p_screen_name);

public:
	String get_plugin_name() const override;
	bool has_main_screen() const override;
	const Ref<Texture2D> get_plugin_icon() const override;
	void make_visible(bool p_visible) override;

	PixelPenEditorPlugin();
	~PixelPenEditorPlugin();
};
