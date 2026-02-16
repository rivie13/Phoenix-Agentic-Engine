/**************************************************************************/
/*  assistant_settings_dialog.cpp                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                     PHOENIX AGENTIC GAME ENGINE                        */
/*                     Based on the Godot Engine                          */
/*                       https://godotengine.org                          */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/* Copyright (c) 2026-present Phoenix Agentic Game Engine contributors     */
/* (see AUTHORS.md).                                                       */
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

#include "assistant_settings_dialog.h"

#include "core/object/class_db.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"

void UltimateAISettingsDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_add_pressed"), &UltimateAISettingsDialog::_on_add_pressed);
	ClassDB::bind_method(D_METHOD("_on_remove_pressed"), &UltimateAISettingsDialog::_on_remove_pressed);
}

UltimateAISettingsDialog::UltimateAISettingsDialog() {
	set_title(TTR("Assistant Settings"));
	set_ok_button_text(TTR("Apply"));
	set_min_size(Size2(460, 360));

	VBoxContainer *root = memnew(VBoxContainer);
	add_child(root);

	Label *models_label = memnew(Label);
	models_label->set_text(TTR("Available models"));
	root->add_child(models_label);

	model_list = memnew(ItemList);
	model_list->set_select_mode(ItemList::SELECT_MULTI);
	model_list->set_allow_reselect(true);
	model_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(model_list);

	HBoxContainer *add_row = memnew(HBoxContainer);
	root->add_child(add_row);

	model_input = memnew(LineEdit);
	model_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	model_input->set_placeholder(TTR("Add model id (ex: gpt-5.2-codex)"));
	add_row->add_child(model_input);

	add_button = memnew(Button);
	add_button->set_text(TTR("Add"));
	add_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAISettingsDialog::_on_add_pressed));
	add_row->add_child(add_button);

	remove_button = memnew(Button);
	remove_button->set_text(TTR("Remove Selected"));
	remove_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAISettingsDialog::_on_remove_pressed));
	root->add_child(remove_button);

	root->add_child(memnew(HSeparator));

	Label *service_label = memnew(Label);
	service_label->set_text(TTR("Service mode"));
	root->add_child(service_label);

	service_mode = memnew(OptionButton);
	service_mode->add_item(TTR("Managed via Gateway (default)"));
	service_mode->add_item(TTR("BYOK"));
	service_mode->add_item(TTR("Local Gateway"));
	service_mode->select(0);
	root->add_child(service_mode);

	Label *base_url_label = memnew(Label);
	base_url_label->set_text(TTR("Backend base URL"));
	root->add_child(base_url_label);

	base_url = memnew(LineEdit);
	base_url->set_placeholder(TTR("https://localhost:5244"));
	root->add_child(base_url);

	Label *managed_label = memnew(Label);
	managed_label->set_text(TTR("Managed endpoint"));
	root->add_child(managed_label);

	managed_endpoint = memnew(LineEdit);
	managed_endpoint->set_placeholder(TTR("https://gateway.phoenix.local"));
	root->add_child(managed_endpoint);

	Label *byok_label = memnew(Label);
	byok_label->set_text(TTR("BYOK API key"));
	root->add_child(byok_label);

	byok_key = memnew(LineEdit);
	byok_key->set_placeholder(TTR("sk-..."));
	byok_key->set_secret(true);
	root->add_child(byok_key);

	Label *local_label = memnew(Label);
	local_label->set_text(TTR("Local endpoint"));
	root->add_child(local_label);

	local_endpoint = memnew(LineEdit);
	local_endpoint->set_placeholder(TTR("http://localhost:5244"));
	root->add_child(local_endpoint);

	Label *token_hook_label = memnew(Label);
	token_hook_label->set_text(TTR("Token hook (literal or env:VAR_NAME)"));
	root->add_child(token_hook_label);

	token_hook = memnew(LineEdit);
	token_hook->set_placeholder(TTR("env:PHOENIX_API_TOKEN"));
	root->add_child(token_hook);

	Label *actor_label = memnew(Label);
	actor_label->set_text(TTR("Actor ID header (x-phoenix-actor-id)"));
	root->add_child(actor_label);

	actor_id = memnew(LineEdit);
	actor_id->set_placeholder(TTR("editor-user"));
	root->add_child(actor_id);

	Label *tier_label = memnew(Label);
	tier_label->set_text(TTR("Tier header (x-phoenix-tier)"));
	root->add_child(tier_label);

	tier = memnew(LineEdit);
	tier->set_placeholder(TTR("managed"));
	root->add_child(tier);

	HBoxContainer *transport_row = memnew(HBoxContainer);
	root->add_child(transport_row);

	Label *timeout_label = memnew(Label);
	timeout_label->set_text(TTR("Timeout (ms)"));
	transport_row->add_child(timeout_label);

	timeout_ms = memnew(SpinBox);
	timeout_ms->set_min(1000);
	timeout_ms->set_max(120000);
	timeout_ms->set_step(500);
	timeout_ms->set_value(15000);
	timeout_ms->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	transport_row->add_child(timeout_ms);

	Label *retry_label = memnew(Label);
	retry_label->set_text(TTR("Retry"));
	transport_row->add_child(retry_label);

	retry_count = memnew(SpinBox);
	retry_count->set_min(0);
	retry_count->set_max(5);
	retry_count->set_step(1);
	retry_count->set_value(1);
	retry_count->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	transport_row->add_child(retry_count);

	root->add_child(memnew(HSeparator));

	Label *rules_label = memnew(Label);
	rules_label->set_text(TTR("Rules"));
	root->add_child(rules_label);

	allow_background_agents = memnew(CheckBox);
	allow_background_agents->set_text(TTR("Allow background agents"));
	allow_background_agents->set_pressed(true);
	root->add_child(allow_background_agents);

	auto_approve_reads = memnew(CheckBox);
	auto_approve_reads->set_text(TTR("Auto-approve read tools"));
	auto_approve_reads->set_pressed(true);
	root->add_child(auto_approve_reads);

	require_approvals = memnew(CheckBox);
	require_approvals->set_text(TTR("Require approvals for write tools"));
	require_approvals->set_pressed(true);
	root->add_child(require_approvals);

	require_signed_commands = memnew(CheckBox);
	require_signed_commands->set_text(TTR("Require signatures for executable commands"));
	require_signed_commands->set_pressed(true);
	root->add_child(require_signed_commands);
}

void UltimateAISettingsDialog::set_models(const PackedStringArray &p_models) {
	model_list->clear();
	for (int i = 0; i < p_models.size(); i++) {
		model_list->add_item(p_models[i]);
		model_list->select(i, false);
	}
}

PackedStringArray UltimateAISettingsDialog::get_selected_models() const {
	PackedStringArray selected;
	PackedInt32Array indices = model_list->get_selected_items();
	for (int i = 0; i < indices.size(); i++) {
		int idx = indices[i];
		selected.push_back(model_list->get_item_text(idx));
	}
	return selected;
}

void UltimateAISettingsDialog::set_runtime_config(const Dictionary &p_config) {
	if (p_config.has("auth_mode") && service_mode) {
		String auth_mode_value = String(p_config["auth_mode"]).to_lower();
		if (auth_mode_value == "byok") {
			service_mode->select(1);
		} else if (auth_mode_value == "local") {
			service_mode->select(2);
		} else {
			service_mode->select(0);
		}
	}
	if (p_config.has("base_url") && base_url) {
		base_url->set_text(String(p_config["base_url"]));
	}
	if (p_config.has("token") && byok_key) {
		byok_key->set_text(String(p_config["token"]));
	}
	if (p_config.has("token_hook") && token_hook) {
		token_hook->set_text(String(p_config["token_hook"]));
	}
	if (p_config.has("actor_id") && actor_id) {
		actor_id->set_text(String(p_config["actor_id"]));
	}
	if (p_config.has("tier") && tier) {
		tier->set_text(String(p_config["tier"]));
	}
	if (p_config.has("timeout_ms") && timeout_ms) {
		timeout_ms->set_value(int(p_config["timeout_ms"]));
	}
	if (p_config.has("retry_count") && retry_count) {
		retry_count->set_value(int(p_config["retry_count"]));
	}
	if (p_config.has("require_signed_commands") && require_signed_commands) {
		require_signed_commands->set_pressed(bool(p_config["require_signed_commands"]));
	}
}

Dictionary UltimateAISettingsDialog::get_runtime_config() const {
	Dictionary config;

	String auth_mode = "managed";
	if (service_mode) {
		int selected = service_mode->get_selected();
		if (selected == 1) {
			auth_mode = "byok";
		} else if (selected == 2) {
			auth_mode = "local";
		}
	}

	String effective_base_url;
	if (base_url) {
		effective_base_url = base_url->get_text().strip_edges();
	}
	if (effective_base_url.is_empty()) {
		if (auth_mode == "local" && local_endpoint) {
			effective_base_url = local_endpoint->get_text().strip_edges();
		} else if (managed_endpoint) {
			effective_base_url = managed_endpoint->get_text().strip_edges();
		}
	}
	if (effective_base_url.is_empty()) {
		effective_base_url = auth_mode == "local" ? "http://localhost:5244" : "https://localhost:5244";
	}

	config["auth_mode"] = auth_mode;
	config["base_url"] = effective_base_url;
	config["token"] = byok_key ? byok_key->get_text().strip_edges() : String();
	config["token_hook"] = token_hook ? token_hook->get_text().strip_edges() : String();
	config["actor_id"] = actor_id ? actor_id->get_text().strip_edges() : String();
	config["tier"] = tier ? tier->get_text().strip_edges() : String();
	config["timeout_ms"] = timeout_ms ? int(timeout_ms->get_value()) : 15000;
	config["retry_count"] = retry_count ? int(retry_count->get_value()) : 1;
	config["require_signed_commands"] = require_signed_commands ? require_signed_commands->is_pressed() : true;

	PackedStringArray allowlist;
	allowlist.push_back("chat_message");
	config["command_allowlist"] = allowlist;

	return config;
}

void UltimateAISettingsDialog::_on_add_pressed() {
	String new_model = model_input->get_text().strip_edges();
	if (new_model.is_empty()) {
		return;
	}
	int existing = -1;
	int item_count = model_list->get_item_count();
	for (int i = 0; i < item_count; i++) {
		if (model_list->get_item_text(i) == new_model) {
			existing = i;
			break;
		}
	}
	if (existing >= 0) {
		model_list->select(existing, true);
		model_input->clear();
		return;
	}

	int idx = model_list->add_item(new_model);
	model_list->select(idx, true);
	model_input->clear();
}

void UltimateAISettingsDialog::_on_remove_pressed() {
	PackedInt32Array indices = model_list->get_selected_items();
	for (int i = indices.size() - 1; i >= 0; i--) {
		model_list->remove_item(indices[i]);
	}
}
