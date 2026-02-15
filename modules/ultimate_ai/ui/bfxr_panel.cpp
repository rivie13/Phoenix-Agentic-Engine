/**************************************************************************/
/*  bfxr_panel.cpp                                                        */
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

#include "bfxr_panel.h"

#include "../core/bfxr_runtime_bridge.h"
#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/math/math_funcs.h"
#include "core/object/class_db.h"
#include "core/string/translation.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/slider.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/box_container.h"
#include "scene/resources/audio_stream_wav.h"

namespace {
const char *const BFXR_PREVIEW_FILE = "user://.phoenix_bfxr/preview.wav";
const char *const BFXR_DEFAULT_IMPORT_FILE = "res://audio/sfx/bfxr_generated.wav";

Vector<uint8_t> _decode_base64_bytes(const String &p_base64) {
	if (p_base64.is_empty()) {
		return Vector<uint8_t>();
	}

	CharString cstr = p_base64.ascii();
	const int str_len = p_base64.length();
	Vector<uint8_t> bytes;
	bytes.resize(str_len / 4 * 3 + 1);

	size_t decoded_len = 0;
	uint8_t *write = bytes.ptrw();
	Error err = CryptoCore::b64_decode(write, bytes.size(), &decoded_len, (const uint8_t *)cstr.get_data(), str_len);
	if (err != OK) {
		return Vector<uint8_t>();
	}

	bytes.resize(decoded_len);
	return bytes;
}
}

void BfxrPanel::_bind_methods() {
}

BfxrPanel::BfxrPanel() {
	runtime_bridge.instantiate();

	set_name("BFXRPanel");
	set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	VBoxContainer *root = memnew(VBoxContainer);
	add_child(root);
	root->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	Label *title = memnew(Label);
	title->set_text(TTR("BFXR Sound Designer"));
	root->add_child(title);

	HBoxContainer *tabs_actions_row = memnew(HBoxContainer);
	tabs_actions_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(tabs_actions_row);

	Button *new_tab_button = memnew(Button);
	new_tab_button->set_text(TTR("New Sound Tab"));
	new_tab_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_new_tab_pressed));
	tabs_actions_row->add_child(new_tab_button);

	close_tab_button = memnew(Button);
	close_tab_button->set_text(TTR("Close Tab"));
	close_tab_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_close_tab_pressed));
	tabs_actions_row->add_child(close_tab_button);

	sound_tabs = memnew(TabContainer);
	sound_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sound_tabs->set_custom_minimum_size(Size2(0, 34));
	sound_tabs->connect("tab_changed", callable_mp(this, &BfxrPanel::_on_tab_changed));
	root->add_child(sound_tabs);

	HBoxContainer *selectors_row = memnew(HBoxContainer);
	selectors_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(selectors_row);

	Label *synth_label = memnew(Label);
	synth_label->set_text(TTR("Synth"));
	selectors_row->add_child(synth_label);

	synth_selector = memnew(OptionButton);
	synth_selector->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	synth_selector->set_tooltip_text(TTR("Select the synthesis engine. Bfxr is broad retro SFX; Footsteppr specializes in footsteps."));
	selectors_row->add_child(synth_selector);
	synth_selector->connect("item_selected", callable_mp(this, &BfxrPanel::_on_synth_selected));

	Label *preset_label = memnew(Label);
	preset_label->set_text(TTR("Preset"));
	selectors_row->add_child(preset_label);

	preset_selector = memnew(OptionButton);
	preset_selector->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	preset_selector->set_tooltip_text(TTR("Choose a preset generator for the selected synth. Selecting a preset auto-generates and plays."));
	selectors_row->add_child(preset_selector);
	preset_selector->connect("item_selected", callable_mp(this, &BfxrPanel::_on_preset_selected));

	Label *seed_label = memnew(Label);
	seed_label->set_text(TTR("Seed"));
	selectors_row->add_child(seed_label);

	seed_edit = memnew(LineEdit);
	seed_edit->set_placeholder(TTR("Optional"));
	seed_edit->set_tooltip_text(TTR("Optional deterministic seed. Same synth + preset + seed yields repeatable results."));
	seed_edit->set_custom_minimum_size(Size2(110, 0));
	selectors_row->add_child(seed_edit);
	seed_edit->connect("text_changed", callable_mp(this, &BfxrPanel::_on_seed_changed));

	synth_info_label = memnew(Label);
	synth_info_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	synth_info_label->set_clip_text(true);
	root->add_child(synth_info_label);

	preset_info_label = memnew(Label);
	preset_info_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	preset_info_label->set_clip_text(true);
	root->add_child(preset_info_label);

	HBoxContainer *actions_row = memnew(HBoxContainer);
	actions_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(actions_row);

	Button *refresh_button = memnew(Button);
	refresh_button->set_text(TTR("Refresh"));
	refresh_button->set_tooltip_text(TTR("Reload available synths, presets, and parameter metadata from the BFXR runtime bridge."));
	refresh_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_refresh_pressed));
	actions_row->add_child(refresh_button);

	Button *randomize_button = memnew(Button);
	randomize_button->set_text(TTR("Randomize Variant"));
	randomize_button->set_tooltip_text(TTR("Generate a new variation within the currently selected synth/preset without changing preset category."));
	randomize_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_randomize_pressed));
	actions_row->add_child(randomize_button);

	Button *random_context_button = memnew(Button);
	random_context_button->set_text(TTR("Random Synth/Preset"));
	random_context_button->set_tooltip_text(TTR("Pick a random synth and preset, then auto-generate and play."));
	random_context_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_randomize_context_pressed));
	actions_row->add_child(random_context_button);

	Button *mutate_button = memnew(Button);
	mutate_button->set_text(TTR("Mutate"));
	mutate_button->set_tooltip_text(TTR("Small random changes to current parameters for iterative tweaking."));
	mutate_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_mutate_pressed));
	actions_row->add_child(mutate_button);

	Button *generate_button = memnew(Button);
	generate_button->set_text(TTR("Generate WAV"));
	generate_button->set_tooltip_text(TTR("Generate WAV from current synth, preset, seed, and slider/JSON values."));
	generate_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_generate_pressed));
	actions_row->add_child(generate_button);

	Button *preview_button = memnew(Button);
	preview_button->set_text(TTR("Play"));
	preview_button->set_tooltip_text(TTR("Play current settings. Regenerates automatically if sliders/JSON changed."));
	preview_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_preview_pressed));
	actions_row->add_child(preview_button);

	Button *stop_button = memnew(Button);
	stop_button->set_text(TTR("Stop"));
	stop_button->set_tooltip_text(TTR("Stop current playback."));
	stop_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_stop_pressed));
	actions_row->add_child(stop_button);

	HSplitContainer *split = memnew(HSplitContainer);
	split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(split);

	ScrollContainer *params_scroll = memnew(ScrollContainer);
	params_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	params_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	split->add_child(params_scroll);

	params_controls_column = memnew(VBoxContainer);
	params_controls_column->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	params_controls_column->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	params_scroll->add_child(params_controls_column);

	VBoxContainer *params_editor_column = memnew(VBoxContainer);
	params_editor_column->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	params_editor_column->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	split->add_child(params_editor_column);

	Label *params_label = memnew(Label);
	params_label->set_text(TTR("Parameter Overrides (JSON object)"));
	params_editor_column->add_child(params_label);

	params_json_edit = memnew(TextEdit);
	params_json_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	params_json_edit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	params_json_edit->set_text("{}");
	params_json_edit->connect("text_changed", callable_mp(this, &BfxrPanel::_on_params_json_changed));
	params_editor_column->add_child(params_json_edit);

	HBoxContainer *import_row = memnew(HBoxContainer);
	import_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(import_row);

	Label *import_label = memnew(Label);
	import_label->set_text(TTR("Import Path"));
	import_row->add_child(import_label);

	import_path_edit = memnew(LineEdit);
	import_path_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	import_path_edit->set_text(BFXR_DEFAULT_IMPORT_FILE);
	import_row->add_child(import_path_edit);

	Button *import_button = memnew(Button);
	import_button->set_text(TTR("Import WAV"));
	import_button->connect("pressed", callable_mp(this, &BfxrPanel::_on_import_pressed));
	import_row->add_child(import_button);

	status_label = memnew(Label);
	status_label->set_clip_text(true);
	root->add_child(status_label);

	preview_player = memnew(AudioStreamPlayer);
	add_child(preview_player);

	_refresh_synths();
	_add_sound_tab(TTR("SFX 1"));
	_update_tab_buttons();
	_set_status(TTR("BFXR panel ready."));
}

void BfxrPanel::_set_param_display_value(int p_index, double p_value) {
	if (p_index < 0 || p_index >= param_controls.size()) {
		return;
	}
	ParamControl &control = param_controls.write[p_index];
	if (!control.value_label) {
		return;
	}
	if (control.integer) {
		control.value_label->set_text(itos((int)Math::round(p_value)));
	} else {
		control.value_label->set_text(String::num(p_value, 3));
	}
}

void BfxrPanel::_sync_params_json_from_sliders() {
	if (suppress_param_sync || !params_json_edit) {
		return;
	}

	Dictionary params;
	Variant parsed_existing = JSON::parse_string(params_json_edit->get_text().strip_edges());
	if (parsed_existing.get_type() == Variant::DICTIONARY) {
		params = parsed_existing;
	}

	for (int i = 0; i < param_controls.size(); i++) {
		const ParamControl &control = param_controls[i];
		if (!control.slider) {
			continue;
		}
		double value = control.slider->get_value();
		if (control.integer) {
			params[control.name] = (int)Math::round(value);
		} else {
			params[control.name] = value;
		}
	}

	suppress_param_sync = true;
	params_json_edit->set_text(JSON::stringify(params, "\t"));
	suppress_param_sync = false;
}

Dictionary BfxrPanel::_parse_params_json_text() const {
	if (!params_json_edit) {
		return Dictionary();
	}

	const String params_text = params_json_edit->get_text().strip_edges();
	if (params_text.is_empty()) {
		return Dictionary();
	}

	Variant parsed = JSON::parse_string(params_text);
	if (parsed.get_type() == Variant::DICTIONARY) {
		return parsed;
	}

	return Dictionary();
}

void BfxrPanel::_apply_params_dictionary_to_sliders(const Dictionary &p_params) {
	if (p_params.is_empty()) {
		return;
	}

	suppress_param_sync = true;
	for (int i = 0; i < param_controls.size(); i++) {
		ParamControl &control = param_controls.write[i];
		if (!control.slider || !p_params.has(control.name)) {
			continue;
		}

		Variant v = p_params.get(control.name, Variant());
		double value = control.slider->get_value();
		if (v.get_type() == Variant::INT || v.get_type() == Variant::FLOAT) {
			value = (double)v;
		} else if (v.get_type() == Variant::STRING) {
			String text_value = String(v).strip_edges();
			if (text_value.is_valid_float()) {
				value = text_value.to_float();
			}
		}

		value = CLAMP(value, control.min_value, control.max_value);
		if (control.integer) {
			value = Math::round(value);
		}

		control.slider->set_value(value);
		_set_param_display_value(i, value);
	}
	suppress_param_sync = false;
}

void BfxrPanel::_sync_sliders_from_params_json() {
	Dictionary params = _parse_params_json_text();
	if (params.is_empty()) {
		return;
	}
	_apply_params_dictionary_to_sliders(params);
}

void BfxrPanel::_update_synth_description() {
	if (!synth_info_label) {
		return;
	}

	const String synth_id = _get_selected_synth_id();
	String synth_name = synth_id;
	if (synth_selector && synth_selector->get_selected() >= 0) {
		synth_name = synth_selector->get_item_text(synth_selector->get_selected());
	}

	String description = synth_descriptions.get(synth_id, "");
	if (description.is_empty()) {
		description = _get_synth_fallback_description(synth_id);
	}

	synth_info_label->set_text(vformat(TTR("Synth: %s - %s"), synth_name, description));
}

void BfxrPanel::_update_preset_description() {
	if (!preset_info_label) {
		return;
	}

	const String preset_id = _get_selected_preset_id();
	if (preset_id.is_empty()) {
		preset_info_label->set_text(TTR("Preset: (none) - Uses current sliders/JSON directly."));
		return;
	}

	String description = preset_descriptions.get(preset_id, "");
	if (description.is_empty()) {
		description = TTR("Preset generator without description.");
	}

	String preset_name = preset_id;
	if (preset_selector && preset_selector->get_selected() >= 0) {
		preset_name = preset_selector->get_item_text(preset_selector->get_selected());
	}

	preset_info_label->set_text(vformat(TTR("Preset: %s - %s"), preset_name, description));
}

String BfxrPanel::_get_synth_fallback_description(const String &p_synth_id) const {
	if (p_synth_id == "bfxr") {
		return TTR("General-purpose retro game SFX synth (laser, UI blips, impacts, pickups).");
	}
	if (p_synth_id == "footsteppr") {
		return TTR("Specialized physical footstep synthesizer with terrain and gait controls.");
	}
	if (p_synth_id == "transfxr") {
		return TTR("Transition/foley-oriented synth variant. Runtime support may be unavailable.");
	}
	return TTR("Synth description unavailable.");
}

String BfxrPanel::_get_param_fallback_description(const String &p_synth_id, const String &p_param_name) const {
	if (p_synth_id == "footsteppr") {
		if (p_param_name == "terrain") {
			return TTR("Selects the surface model used to synthesize footsteps.");
		}
		if (p_param_name == "heel") {
			return TTR("Strength of heel impact at the start of the step.");
		}
		if (p_param_name == "roll") {
			return TTR("How much the step rolls through the mid-foot phase.");
		}
		if (p_param_name == "ball") {
			return TTR("Pressure at toe-off / front-foot phase.");
		}
		if (p_param_name == "swiftness") {
			return TTR("Controls step speed and overall step duration.");
		}
	}

	if (p_param_name == "masterVolume") {
		return TTR("Overall output level.");
	}
	if (p_param_name == "waveType") {
		return TTR("Chooses the waveform family/timbre for the sound.");
	}
	if (p_param_name == "attackTime") {
		return TTR("Fade-in time at the start of the sound.");
	}
	if (p_param_name == "sustainTime") {
		return TTR("Main hold portion of the envelope.");
	}
	if (p_param_name == "decayTime") {
		return TTR("Fade-out tail duration.");
	}
	if (p_param_name == "frequency_start") {
		return TTR("Starting pitch of the generated sound.");
	}
	if (p_param_name == "frequency_slide") {
		return TTR("Pitch drift over time (up/down).");
	}
	if (p_param_name == "vibratoDepth") {
		return TTR("How strong pitch wobble (vibrato) is.");
	}
	if (p_param_name == "vibratoSpeed") {
		return TTR("How fast vibrato oscillates.");
	}
	if (p_param_name == "bitCrush") {
		return TTR("Reduces sample detail for lo-fi digital texture.");
	}

	return TTR("Adjusts this synthesis parameter.");
}

String BfxrPanel::_build_param_tooltip(const Dictionary &p_param, const String &p_synth_id) const {
	const String name = p_param.get("name", "");
	const String display_name = p_param.get("displayName", name);
	String description = p_param.get("description", "");
	if (description.is_empty()) {
		description = _get_param_fallback_description(p_synth_id, name);
	}

	const Variant min_var = p_param.get("min", 0.0);
	const Variant max_var = p_param.get("max", 1.0);
	const Variant default_var = p_param.get("default", 0.0);
	const String type = String(p_param.get("type", "RANGE"));

	String tooltip = vformat(TTR("%s\n%s\nRange: %s to %s\nDefault: %s"),
			display_name,
			description,
			String(min_var),
			String(max_var),
			String(default_var));

	if (type == "BUTTONSELECT" && p_param.has("values") && p_param["values"].get_type() == Variant::ARRAY) {
		Array values = p_param["values"];
		if (!values.is_empty()) {
			tooltip += vformat(TTR("\nType: Enum (%d options)"), values.size());
		}
	}

	return tooltip;
}

int BfxrPanel::_find_selector_index_by_metadata(const OptionButton *p_selector, const String &p_id) const {
	if (!p_selector || p_id.is_empty()) {
		return -1;
	}

	for (int i = 0; i < p_selector->get_item_count(); i++) {
		Variant item_metadata = p_selector->get_item_metadata(i);
		if (item_metadata.get_type() == Variant::STRING && String(item_metadata) == p_id) {
			return i;
		}
	}

	return -1;
}

void BfxrPanel::_save_active_tab_state() {
	if (active_sound_tab < 0 || active_sound_tab >= sound_tab_states.size()) {
		return;
	}

	SoundTabState &state = sound_tab_states.write[active_sound_tab];
	state.synth_id = _get_selected_synth_id();
	state.preset_id = _get_selected_preset_id();
	state.seed_text = seed_edit ? seed_edit->get_text() : String();
	state.params_json_text = params_json_edit ? params_json_edit->get_text() : String("{}");
	state.import_path = import_path_edit ? import_path_edit->get_text().strip_edges() : String(BFXR_DEFAULT_IMPORT_FILE);
	state.wav_data = last_wav_data;
	state.generation_result = last_generation_result;
}

void BfxrPanel::_load_tab_state(int p_tab_index) {
	if (p_tab_index < 0 || p_tab_index >= sound_tab_states.size()) {
		return;
	}

	const SoundTabState &state = sound_tab_states[p_tab_index];
	suppress_tab_events = true;

	suppress_param_sync = true;
	if (params_json_edit) {
		params_json_edit->set_text(state.params_json_text.strip_edges().is_empty() ? String("{}") : state.params_json_text);
	}
	suppress_param_sync = false;

	int synth_index = _find_selector_index_by_metadata(synth_selector, state.synth_id);
	if (synth_index < 0 && synth_selector && synth_selector->get_item_count() > 0) {
		synth_index = 0;
	}
	if (synth_index >= 0) {
		synth_selector->select(synth_index);
	}

	_refresh_presets();
	_refresh_params();

	int preset_index = _find_selector_index_by_metadata(preset_selector, state.preset_id);
	if (preset_index < 0 && preset_selector && preset_selector->get_item_count() > 0) {
		preset_index = 0;
	}
	if (preset_index >= 0) {
		preset_selector->select(preset_index);
	}

	if (seed_edit) {
		seed_edit->set_text(state.seed_text);
	}
	if (import_path_edit) {
		import_path_edit->set_text(state.import_path.strip_edges().is_empty() ? String(BFXR_DEFAULT_IMPORT_FILE) : state.import_path);
	}

	last_wav_data = state.wav_data;
	last_generation_result = state.generation_result;
	preview_needs_regen = last_wav_data.is_empty();
	_update_synth_description();
	_update_preset_description();
	suppress_tab_events = false;
}

void BfxrPanel::_add_sound_tab(const String &p_title) {
	_save_active_tab_state();

	SoundTabState state;
	state.title = p_title;
	state.synth_id = _get_selected_synth_id();
	state.preset_id = _get_selected_preset_id();
	state.seed_text = "";
	state.params_json_text = params_json_edit ? params_json_edit->get_text() : String("{}");
	if (state.params_json_text.strip_edges().is_empty()) {
		state.params_json_text = "{}";
	}
	state.import_path = BFXR_DEFAULT_IMPORT_FILE;

	if (state.title.is_empty()) {
		state.title = vformat(TTR("SFX %d"), sound_tab_states.size() + 1);
	}

	sound_tab_states.push_back(state);

	Control *tab_page = memnew(Control);
	tab_page->set_name(state.title);
	tab_page->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab_page->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	sound_tabs->add_child(tab_page);

	const int tab_index = sound_tabs->get_tab_count() - 1;
	sound_tabs->set_tab_title(tab_index, state.title);

	suppress_tab_events = true;
	sound_tabs->set_current_tab(tab_index);
	suppress_tab_events = false;

	active_sound_tab = tab_index;
	_load_tab_state(active_sound_tab);
	_update_tab_buttons();
}

void BfxrPanel::_update_tab_buttons() {
	if (close_tab_button) {
		close_tab_button->set_disabled(sound_tab_states.size() <= 1);
	}
}

void BfxrPanel::_set_status(const String &p_text, bool p_is_error) {
	if (!status_label) {
		return;
	}
	if (p_is_error) {
		status_label->set_text(vformat(TTR("Error: %s"), p_text));
	} else {
		status_label->set_text(p_text);
	}
}

String BfxrPanel::_get_selected_synth_id() const {
	if (!synth_selector || synth_selector->get_item_count() == 0) {
		return "bfxr";
	}
	const int selected = synth_selector->get_selected();
	if (selected < 0) {
		return "bfxr";
	}
	const Variant item_metadata = synth_selector->get_item_metadata(selected);
	if (item_metadata.get_type() == Variant::STRING) {
		return item_metadata;
	}
	return synth_selector->get_item_text(selected).to_lower();
}

String BfxrPanel::_get_selected_preset_id() const {
	if (!preset_selector || preset_selector->get_item_count() == 0) {
		return "";
	}
	const int selected = preset_selector->get_selected();
	if (selected < 0) {
		return "";
	}
	const Variant item_metadata = preset_selector->get_item_metadata(selected);
	if (item_metadata.get_type() == Variant::STRING) {
		return item_metadata;
	}
	return "";
}

bool BfxrPanel::_extract_command_result(const Dictionary &p_response, Variant &r_result, String &r_error) const {
	r_error = "";
	r_result = Variant();

	if (p_response.has("ok")) {
		const bool ok = bool(p_response.get("ok", false));
		if (!ok) {
			r_error = p_response.get("error", TTR("Unknown bridge error"));
			return false;
		}
		r_result = p_response.get("result", Variant());
		if (r_result.get_type() == Variant::NIL && p_response.has("data")) {
			r_result = p_response.get("data", Variant());
		}
		return true;
	}

	if (p_response.has("error")) {
		r_error = p_response.get("error", TTR("Unknown bridge error"));
		return false;
	}

	r_result = p_response;
	return true;
}

void BfxrPanel::_refresh_synths() {
	synth_selector->clear();
	synth_descriptions.clear();

	Variant result;
	String error;
	if (_extract_command_result(runtime_bridge->list_synths(), result, error) && result.get_type() == Variant::ARRAY) {
		Array synths = result;
		for (int i = 0; i < synths.size(); i++) {
			Dictionary synth = synths[i];
			String synth_id = synth.get("id", "");
			String synth_name = synth.get("name", synth_id);
			String synth_description = synth.get("description", "");
			if (synth_id.is_empty()) {
				continue;
			}
			const int item_idx = synth_selector->get_item_count();
			synth_selector->add_item(synth_name);
			synth_selector->set_item_metadata(item_idx, synth_id);
			if (synth_description.is_empty()) {
				synth_description = _get_synth_fallback_description(synth_id);
			}
			synth_descriptions[synth_id] = synth_description;
		}
	} else {
		if (!error.is_empty()) {
			_set_status(error, true);
		}
	}

	if (synth_selector->get_item_count() == 0) {
		const String fallback_ids[2] = { "bfxr", "footsteppr" };
		const String fallback_names[2] = { "Bfxr", "Footsteppr" };
		for (int i = 0; i < 2; i++) {
			const int item_idx = synth_selector->get_item_count();
			synth_selector->add_item(fallback_names[i]);
			synth_selector->set_item_metadata(item_idx, fallback_ids[i]);
			synth_descriptions[fallback_ids[i]] = _get_synth_fallback_description(fallback_ids[i]);
		}
	}

	synth_selector->select(0);
	_refresh_presets();
	_refresh_params();
	_update_synth_description();
}

void BfxrPanel::_refresh_presets() {
	preset_selector->clear();
	preset_descriptions.clear();

	Dictionary response = runtime_bridge->list_presets(_get_selected_synth_id());
	Variant result;
	String error;
	if (!_extract_command_result(response, result, error) || result.get_type() != Variant::ARRAY) {
		if (!error.is_empty()) {
			_set_status(error, true);
		}
		preset_selector->add_item(TTR("(none)"));
		preset_selector->set_item_metadata(0, "");
		preset_selector->select(0);
		_update_preset_description();
		return;
	}

	Array presets = result;
	for (int i = 0; i < presets.size(); i++) {
		Dictionary preset = presets[i];
		String preset_id = preset.get("id", "");
		String preset_name = preset.get("name", preset_id);
		String preset_description = preset.get("description", "");
		String preset_source = preset.get("source", "");
		if (preset_id.is_empty()) {
			continue;
		}
		const int item_idx = preset_selector->get_item_count();
		preset_selector->add_item(preset_name);
		preset_selector->set_item_metadata(item_idx, preset_id);
		if (!preset_source.is_empty()) {
			if (preset_description.is_empty()) {
				preset_description = vformat(TTR("Source: %s"), preset_source);
			} else {
				preset_description += vformat(TTR(" (source: %s)"), preset_source);
			}
		}
		preset_descriptions[preset_id] = preset_description;
	}

	if (preset_selector->get_item_count() == 0) {
		preset_selector->add_item(TTR("(none)"));
		preset_selector->set_item_metadata(0, "");
	}
	preset_selector->select(0);
	_update_preset_description();
}

void BfxrPanel::_refresh_params() {
	if (!params_controls_column) {
		return;
	}

	while (params_controls_column->get_child_count() > 0) {
		Node *child = params_controls_column->get_child(0);
		params_controls_column->remove_child(child);
		child->queue_free();
	}
	param_controls.clear();

	Dictionary response = runtime_bridge->list_params(_get_selected_synth_id());
	Variant result;
	String error;
	if (!_extract_command_result(response, result, error) || result.get_type() != Variant::ARRAY) {
		if (!error.is_empty()) {
			_set_status(error, true);
		}
		return;
	}

	Array params = result;
	const String synth_id = _get_selected_synth_id();
	for (int i = 0; i < params.size(); i++) {
		Dictionary param = params[i];
		const String name = param.get("name", "");
		const String display_name = param.get("displayName", name);
		if (name.is_empty()) {
			continue;
		}

		double min_value = 0.0;
		double max_value = 1.0;
		double default_value = 0.0;

		Variant min_var = param.get("min", 0.0);
		Variant max_var = param.get("max", 1.0);
		Variant default_var = param.get("default", 0.0);

		if (min_var.get_type() == Variant::INT || min_var.get_type() == Variant::FLOAT) {
			min_value = (double)min_var;
		}
		if (max_var.get_type() == Variant::INT || max_var.get_type() == Variant::FLOAT) {
			max_value = (double)max_var;
		}
		if (default_var.get_type() == Variant::INT || default_var.get_type() == Variant::FLOAT) {
			default_value = (double)default_var;
		}

		const String param_type = param.get("type", "RANGE");
		if (param_type == "BUTTONSELECT") {
			max_value = MAX(min_value, max_value - 1.0);
		}

		const bool integer_param =
				(min_var.get_type() == Variant::INT) &&
				(max_var.get_type() == Variant::INT) &&
				(default_var.get_type() == Variant::INT || param_type == "BUTTONSELECT");

		HBoxContainer *row = memnew(HBoxContainer);
		row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		params_controls_column->add_child(row);

		Label *name_label = memnew(Label);
		name_label->set_custom_minimum_size(Size2(130, 0));
		name_label->set_text(display_name);
		name_label->set_tooltip_text(_build_param_tooltip(param, synth_id));
		row->add_child(name_label);

		HSlider *slider = memnew(HSlider);
		slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		slider->set_min(min_value);
		slider->set_max(max_value);
		slider->set_step(integer_param ? 1.0 : 0.001);
		slider->set_value(CLAMP(default_value, min_value, max_value));
		slider->set_tooltip_text(_build_param_tooltip(param, synth_id));
		slider->connect("value_changed", callable_mp(this, &BfxrPanel::_on_param_slider_changed).bind(name));
		row->add_child(slider);

		Label *value_label = memnew(Label);
		value_label->set_custom_minimum_size(Size2(72, 0));
		value_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
		value_label->set_tooltip_text(_build_param_tooltip(param, synth_id));
		row->add_child(value_label);

		ParamControl control;
		control.name = name;
		control.min_value = min_value;
		control.max_value = max_value;
		control.integer = integer_param;
		control.slider = slider;
		control.value_label = value_label;
		param_controls.push_back(control);
		_set_param_display_value(param_controls.size() - 1, slider->get_value());
	}

	_sync_sliders_from_params_json();
	_sync_params_json_from_sliders();
}

Dictionary BfxrPanel::_build_generate_options(const String &p_override_preset, bool p_include_params) const {
	Dictionary options;
	options["synth"] = _get_selected_synth_id();

	const String preset = p_override_preset.is_empty() ? _get_selected_preset_id() : p_override_preset;
	if (!preset.is_empty()) {
		options["preset"] = preset;
	}

	const String seed_text = seed_edit->get_text().strip_edges();
	if (!seed_text.is_empty() && seed_text.is_valid_int()) {
		options["seed"] = seed_text.to_int();
	}

	if (p_include_params) {
		Dictionary params = _parse_params_json_text();
		if (!params.is_empty()) {
			options["params"] = params;
		}
	}

	options["returnDataUri"] = false;
	options["returnBase64"] = true;
	return options;
}

bool BfxrPanel::_generate_current_sound(const String &p_override_preset, bool p_include_params) {
	Dictionary response = runtime_bridge->generate_wav(_build_generate_options(p_override_preset, p_include_params));
	Variant result;
	String error;
	if (!_extract_command_result(response, result, error) || result.get_type() != Variant::DICTIONARY) {
		if (!error.is_empty()) {
			_set_status(error, true);
		} else {
			_set_status(TTR("Invalid generate response payload."), true);
		}
		return false;
	}

	_apply_generation_result(result);
	return true;
}

bool BfxrPanel::_play_last_wav() {
	if (last_wav_data.is_empty()) {
		_set_status(TTR("No generated sound available to play."), true);
		return false;
	}

	if (!_ensure_playback_stream(last_wav_data)) {
		_set_status(TTR("Unable to build playback stream from generated WAV."), true);
		return false;
	}

	preview_player->play();
	return true;
}

void BfxrPanel::_apply_generation_result(const Dictionary &p_payload) {
	last_generation_result = p_payload;
	last_wav_data.clear();

	const String base64 = p_payload.get("wavBase64", "");
	if (base64.is_empty()) {
		_set_status(TTR("Bridge returned no wavBase64 payload."), true);
		return;
	}

	last_wav_data = _decode_base64_bytes(base64);
	if (last_wav_data.is_empty()) {
		_set_status(TTR("Failed to decode generated WAV payload."), true);
		return;
	}

	if (p_payload.has("params")) {
		suppress_param_sync = true;
		params_json_edit->set_text(JSON::stringify(p_payload.get("params", Dictionary()), "\t"));
		suppress_param_sync = false;
		_sync_sliders_from_params_json();
		_sync_params_json_from_sliders();
	}
	preview_needs_regen = false;

	String synth = p_payload.get("synth", _get_selected_synth_id());
	String preset = p_payload.get("preset", "");
	const Variant duration_value = p_payload.get("durationSeconds", 0.0);
	double duration = 0.0;
	if (duration_value.get_type() == Variant::STRING) {
		const String duration_text = String(duration_value).strip_edges();
		if (duration_text.is_valid_float()) {
			duration = duration_text.to_float();
		}
	} else {
		duration = (double)duration_value;
	}
	_set_status(vformat(TTR("Generated %s (%s) %.3fs"), synth, preset.is_empty() ? String("custom") : preset, duration));
	_save_active_tab_state();
}

bool BfxrPanel::_write_bytes_to_path(const String &p_res_path, const Vector<uint8_t> &p_bytes) const {
	if (!p_res_path.begins_with("res://") && !p_res_path.begins_with("user://")) {
		return false;
	}

	const String absolute = ProjectSettings::get_singleton()->globalize_path(p_res_path);
	const Error dir_err = DirAccess::make_dir_recursive_absolute(absolute.get_base_dir());
	if (dir_err != OK) {
		return false;
	}

	Ref<FileAccess> file = FileAccess::open(absolute, FileAccess::WRITE);
	if (file.is_null()) {
		return false;
	}
	file->store_buffer(p_bytes);
	return true;
}

bool BfxrPanel::_ensure_playback_stream(const Vector<uint8_t> &p_wav_bytes) {
	if (p_wav_bytes.is_empty()) {
		return false;
	}
	if (!_write_bytes_to_path(BFXR_PREVIEW_FILE, p_wav_bytes)) {
		return false;
	}

	Dictionary options;
	Ref<AudioStreamWAV> stream = AudioStreamWAV::load_from_file(ProjectSettings::get_singleton()->globalize_path(BFXR_PREVIEW_FILE), options);
	if (!stream.is_valid()) {
		return false;
	}
	preview_player->set_stream(stream);
	return true;
}

void BfxrPanel::_on_refresh_pressed() {
	_save_active_tab_state();
	_refresh_synths();
	if (active_sound_tab >= 0 && active_sound_tab < sound_tab_states.size()) {
		_load_tab_state(active_sound_tab);
	}
	const int synth_count = synth_selector ? synth_selector->get_item_count() : 0;
	const int preset_count = preset_selector ? preset_selector->get_item_count() : 0;
	_set_status(vformat(TTR("Refreshed metadata: %d synth(s), %d preset(s), %d parameter control(s)."), synth_count, preset_count, param_controls.size()));
}

void BfxrPanel::_on_new_tab_pressed() {
	_add_sound_tab(vformat(TTR("SFX %d"), sound_tab_states.size() + 1));
	_set_status(TTR("Created new sound tab."));
}

void BfxrPanel::_on_close_tab_pressed() {
	if (!sound_tabs || sound_tab_states.size() <= 1) {
		return;
	}

	_save_active_tab_state();

	int tab_index = sound_tabs->get_current_tab();
	if (tab_index < 0 || tab_index >= sound_tab_states.size()) {
		tab_index = active_sound_tab;
	}
	if (tab_index < 0 || tab_index >= sound_tab_states.size()) {
		return;
	}

	Control *tab_node = sound_tabs->get_tab_control(tab_index);
	suppress_tab_events = true;
	if (tab_node) {
		sound_tabs->remove_child(tab_node);
		tab_node->queue_free();
	}
	sound_tab_states.remove_at(tab_index);

	const int next_tab = MIN(tab_index, sound_tab_states.size() - 1);
	if (next_tab >= 0) {
		sound_tabs->set_current_tab(next_tab);
		active_sound_tab = next_tab;
	}
	suppress_tab_events = false;

	if (active_sound_tab >= 0) {
		_load_tab_state(active_sound_tab);
	}

	_update_tab_buttons();
	_set_status(TTR("Closed sound tab."));
}

void BfxrPanel::_on_tab_changed(int p_tab_index) {
	if (suppress_tab_events) {
		return;
	}

	if (p_tab_index < 0 || p_tab_index >= sound_tab_states.size()) {
		return;
	}

	_save_active_tab_state();
	active_sound_tab = p_tab_index;
	_load_tab_state(active_sound_tab);
	_set_status(vformat(TTR("Switched to %s"), sound_tab_states[active_sound_tab].title));
}

void BfxrPanel::_on_synth_selected(int p_index) {
	if (suppress_tab_events) {
		return;
	}

	suppress_param_sync = true;
	params_json_edit->set_text("{}");
	suppress_param_sync = false;

	_refresh_presets();
	_refresh_params();
	_update_synth_description();
	_update_preset_description();
	_sync_params_json_from_sliders();
	preview_needs_regen = true;
	_save_active_tab_state();
	_set_status(vformat(TTR("Synth switched to %s."), _get_selected_synth_id()));
}

void BfxrPanel::_on_preset_selected(int p_index) {
	if (suppress_tab_events) {
		return;
	}

	_update_preset_description();
	preview_needs_regen = true;
	_save_active_tab_state();

	const String preset = _get_selected_preset_id();
	if (preset.is_empty()) {
		_set_status(TTR("Preset cleared. Click Play to hear current sliders/JSON settings."));
		return;
	}

	if (!_generate_current_sound("", false)) {
		return;
	}
	if (!_play_last_wav()) {
		return;
	}

	_set_status(vformat(TTR("Preset %s loaded and playing."), preset));
	_save_active_tab_state();
}

void BfxrPanel::_on_seed_changed(const String &p_text) {
	preview_needs_regen = true;
	_save_active_tab_state();
	if (!p_text.is_empty() && !p_text.strip_edges().is_valid_int()) {
		_set_status(TTR("Seed is set but not an integer; it will be ignored."), true);
	}
}

void BfxrPanel::_on_param_slider_changed(double p_value, String p_param_name) {
	if (suppress_param_sync) {
		return;
	}

	for (int i = 0; i < param_controls.size(); i++) {
		const ParamControl &control = param_controls[i];
		if (control.name == p_param_name) {
			_set_param_display_value(i, p_value);
			break;
		}
	}

	_sync_params_json_from_sliders();
	preview_needs_regen = true;
	_save_active_tab_state();
}

void BfxrPanel::_on_params_json_changed() {
	if (suppress_param_sync) {
		return;
	}

	_sync_sliders_from_params_json();
	preview_needs_regen = true;
	_save_active_tab_state();
}

void BfxrPanel::_on_generate_pressed() {
	if (!_generate_current_sound()) {
		return;
	}
	_set_status(TTR("WAV generated. Click Play to audition or Import WAV to save."));
	_save_active_tab_state();
}

void BfxrPanel::_on_randomize_pressed() {
	const String selected_preset = _get_selected_preset_id();
	const bool has_selected_preset = !selected_preset.is_empty();
	const String override_preset = has_selected_preset ? String("mutate") : String("randomize");
	const bool include_params = has_selected_preset;

	if (!_generate_current_sound(override_preset, include_params)) {
		return;
	}
	if (!_play_last_wav()) {
		return;
	}

	if (has_selected_preset) {
		_set_status(vformat(TTR("Randomized within preset %s and playing."), selected_preset));
	} else {
		_set_status(TTR("Randomized current synth and playing."));
	}
	_save_active_tab_state();
}

void BfxrPanel::_on_randomize_context_pressed() {
	if (!synth_selector || synth_selector->get_item_count() == 0) {
		_set_status(TTR("No synths available to randomize."), true);
		return;
	}

	const int synth_index = Math::random(0, synth_selector->get_item_count() - 1);
	synth_selector->select(synth_index);
	_on_synth_selected(synth_index);

	if (!preset_selector || preset_selector->get_item_count() == 0) {
		if (_generate_current_sound("randomize", false) && _play_last_wav()) {
			_set_status(vformat(TTR("Random synth selected (%s), generated and playing."), _get_selected_synth_id()));
		}
		return;
	}

	const int preset_index = Math::random(0, preset_selector->get_item_count() - 1);
	preset_selector->select(preset_index);
	_on_preset_selected(preset_index);
}

void BfxrPanel::_on_mutate_pressed() {
	if (!_generate_current_sound("mutate", true)) {
		return;
	}
	if (!_play_last_wav()) {
		return;
	}
	_set_status(TTR("Mutated current sound and playing."));
	_save_active_tab_state();
}

void BfxrPanel::_on_preview_pressed() {
	if (preview_needs_regen || last_wav_data.is_empty()) {
		if (!_generate_current_sound()) {
			return;
		}
	}

	if (!_play_last_wav()) {
		return;
	}

	_set_status(TTR("Playback started."));
}

void BfxrPanel::_on_stop_pressed() {
	if (preview_player) {
		preview_player->stop();
	}
	_set_status(TTR("Preview playback stopped."));
}

void BfxrPanel::_on_import_pressed() {
	if (last_wav_data.is_empty()) {
		_set_status(TTR("Generate a WAV before importing."), true);
		return;
	}

	String import_path = import_path_edit->get_text().strip_edges();
	if (import_path.is_empty()) {
		import_path = BFXR_DEFAULT_IMPORT_FILE;
	}
	if (!import_path.begins_with("res://")) {
		_set_status(TTR("Import path must start with res://"), true);
		return;
	}
	if (!import_path.to_lower().ends_with(".wav")) {
		import_path += ".wav";
	}

	if (!_write_bytes_to_path(import_path, last_wav_data)) {
		_set_status(vformat(TTR("Failed to write %s"), import_path), true);
		return;
	}

	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->scan_changes();
	}

	import_path_edit->set_text(import_path);
	_save_active_tab_state();
	_set_status(vformat(TTR("Imported WAV to %s"), import_path));
}
