/**************************************************************************/
/*  bfxr_panel.h                                                          */
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

#include "core/object/ref_counted.h"
#include "scene/gui/panel_container.h"

class BfxrRuntimeBridge;
class AudioStreamPlayer;
class Button;
class HSlider;
class Label;
class LineEdit;
class OptionButton;
class TabContainer;
class TextEdit;
class VBoxContainer;

class BfxrPanel : public PanelContainer {
	GDCLASS(BfxrPanel, PanelContainer);

	Ref<BfxrRuntimeBridge> runtime_bridge;

	TabContainer *sound_tabs = nullptr;
	Button *close_tab_button = nullptr;
	OptionButton *synth_selector = nullptr;
	OptionButton *preset_selector = nullptr;
	Label *synth_info_label = nullptr;
	Label *preset_info_label = nullptr;
	LineEdit *seed_edit = nullptr;
	TextEdit *params_json_edit = nullptr;
	VBoxContainer *params_controls_column = nullptr;
	LineEdit *import_path_edit = nullptr;
	Label *status_label = nullptr;
	AudioStreamPlayer *preview_player = nullptr;
	Dictionary synth_descriptions;
	Dictionary preset_descriptions;

	struct ParamControl {
		String name;
		double min_value = 0.0;
		double max_value = 1.0;
		bool integer = false;
		HSlider *slider = nullptr;
		Label *value_label = nullptr;
	};

	struct SoundTabState {
		String title;
		String synth_id;
		String preset_id;
		String seed_text;
		String params_json_text;
		String import_path;
		Vector<uint8_t> wav_data;
		Dictionary generation_result;
	};

	Vector<ParamControl> param_controls;
	Vector<SoundTabState> sound_tab_states;
	int active_sound_tab = -1;
	bool suppress_param_sync = false;
	bool suppress_tab_events = false;
	bool preview_needs_regen = true;

	Vector<uint8_t> last_wav_data;
	Dictionary last_generation_result;

	void _refresh_synths();
	void _refresh_presets();
	void _refresh_params();
	void _set_param_display_value(int p_index, double p_value);
	void _sync_params_json_from_sliders();
	void _apply_params_dictionary_to_sliders(const Dictionary &p_params);
	void _sync_sliders_from_params_json();
	void _update_synth_description();
	void _update_preset_description();
	Dictionary _parse_params_json_text() const;
	void _set_status(const String &p_text, bool p_is_error = false);
	String _get_synth_fallback_description(const String &p_synth_id) const;
	String _get_param_fallback_description(const String &p_synth_id, const String &p_param_name) const;
	String _build_param_tooltip(const Dictionary &p_param, const String &p_synth_id) const;
	String _get_selected_synth_id() const;
	String _get_selected_preset_id() const;
	int _find_selector_index_by_metadata(const OptionButton *p_selector, const String &p_id) const;
	void _save_active_tab_state();
	void _load_tab_state(int p_tab_index);
	void _add_sound_tab(const String &p_title);
	void _update_tab_buttons();
	Dictionary _build_generate_options(const String &p_override_preset = "", bool p_include_params = true) const;
	bool _extract_command_result(const Dictionary &p_response, Variant &r_result, String &r_error) const;
	bool _generate_current_sound(const String &p_override_preset = "", bool p_include_params = true);
	bool _play_last_wav();
	void _apply_generation_result(const Dictionary &p_payload);
	bool _write_bytes_to_path(const String &p_res_path, const Vector<uint8_t> &p_bytes) const;
	bool _ensure_playback_stream(const Vector<uint8_t> &p_wav_bytes);

	void _on_refresh_pressed();
	void _on_new_tab_pressed();
	void _on_close_tab_pressed();
	void _on_tab_changed(int p_tab_index);
	void _on_synth_selected(int p_index);
	void _on_preset_selected(int p_index);
	void _on_seed_changed(const String &p_text);
	void _on_param_slider_changed(double p_value, String p_param_name);
	void _on_params_json_changed();
	void _on_generate_pressed();
	void _on_randomize_pressed();
	void _on_randomize_context_pressed();
	void _on_mutate_pressed();
	void _on_preview_pressed();
	void _on_stop_pressed();
	void _on_import_pressed();

protected:
	static void _bind_methods();

public:
	BfxrPanel();
};
