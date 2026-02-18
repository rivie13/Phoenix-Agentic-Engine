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

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"

namespace {
String _get_env_file_value(const String &p_file_path, const String &p_key) {
	if (p_file_path.is_empty() || p_key.is_empty() || !FileAccess::exists(p_file_path)) {
		return String();
	}

	Ref<FileAccess> env_file = FileAccess::open(p_file_path, FileAccess::READ);
	if (env_file.is_null()) {
		return String();
	}

	while (!env_file->eof_reached()) {
		String line = env_file->get_line().strip_edges();
		if (!line.is_empty() && line.unicode_at(0) == 0xFEFF) {
			line = line.substr(1, line.length() - 1);
		}
		if (line.is_empty() || line.begins_with("#")) {
			continue;
		}

		int separator = line.find("=");
		if (separator <= 0) {
			continue;
		}

		String key = line.substr(0, separator).strip_edges();
		if (key != p_key) {
			continue;
		}

		String value = line.substr(separator + 1, line.length()).strip_edges();
		if (value.length() >= 2) {
			if ((value.begins_with("\"") && value.ends_with("\"")) || (value.begins_with("'") && value.ends_with("'"))) {
				value = value.substr(1, value.length() - 2);
			}
		}
		return value.strip_edges();
	}

	return String();
}

PackedStringArray _collect_env_file_candidates() {
	PackedStringArray candidates;

	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (project_settings) {
		String project_env = project_settings->globalize_path("res://.env.local").simplify_path();
		if (!project_env.is_empty()) {
			candidates.push_back(project_env);
		}
	}

	Ref<DirAccess> current_dir_access = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (current_dir_access.is_valid()) {
		String current_dir = current_dir_access->get_current_dir().simplify_path();
		if (!current_dir.is_empty()) {
			candidates.push_back(current_dir.path_join(".env.local").simplify_path());
		}
	}

	OS *os = OS::get_singleton();
	if (os) {
		String candidate_dir = os->get_executable_path().get_base_dir().simplify_path();
		for (int i = 0; i < 6; i++) {
			if (candidate_dir.is_empty()) {
				break;
			}

			candidates.push_back(candidate_dir.path_join(".env.local").simplify_path());

			String parent = candidate_dir.path_join("..").simplify_path();
			if (parent == candidate_dir) {
				break;
			}
			candidate_dir = parent;
		}
	}

	PackedStringArray deduped;
	for (int i = 0; i < candidates.size(); i++) {
		String candidate = candidates[i].strip_edges();
		if (candidate.is_empty()) {
			continue;
		}
		if (deduped.has(candidate)) {
			continue;
		}
		deduped.push_back(candidate);
	}

	return deduped;
}

String _resolve_gateway_base_url() {
	OS *os = OS::get_singleton();
	if (os) {
		if (os->has_environment("PHOENIX_PUBLIC_GATEWAY_URL")) {
			String value = os->get_environment("PHOENIX_PUBLIC_GATEWAY_URL").strip_edges();
			if (!value.is_empty()) {
				return value;
			}
		}
		if (os->has_environment("PHOENIX_GATEWAY_BASE_URL")) {
			String value = os->get_environment("PHOENIX_GATEWAY_BASE_URL").strip_edges();
			if (!value.is_empty()) {
				return value;
			}
		}
	}

	PackedStringArray candidates = _collect_env_file_candidates();
	for (int i = 0; i < candidates.size(); i++) {
		String value = _get_env_file_value(candidates[i], "PHOENIX_PUBLIC_GATEWAY_URL");
		if (!value.is_empty()) {
			return value;
		}
	}

	for (int i = 0; i < candidates.size(); i++) {
		String value = _get_env_file_value(candidates[i], "PHOENIX_GATEWAY_BASE_URL");
		if (!value.is_empty()) {
			return value;
		}
	}

	return String();
}
} //namespace

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
	base_url->set_placeholder(TTR("https://<your-appservice-host>.azurewebsites.net"));
	String initial_gateway_url = _resolve_gateway_base_url();
	if (!initial_gateway_url.is_empty()) {
		base_url->set_text(initial_gateway_url);
	}
	root->add_child(base_url);

	Label *managed_label = memnew(Label);
	managed_label->set_text(TTR("Managed endpoint"));
	root->add_child(managed_label);

	managed_endpoint = memnew(LineEdit);
	managed_endpoint->set_placeholder(TTR("https://<your-appservice-host>.azurewebsites.net"));
	if (!initial_gateway_url.is_empty()) {
		managed_endpoint->set_text(initial_gateway_url);
	}
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

	show_thinking_stream = memnew(CheckBox);
	show_thinking_stream->set_text(TTR("Show thinking stream (when available)"));
	show_thinking_stream->set_pressed(false);
	root->add_child(show_thinking_stream);

	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (project_settings && project_settings->has_setting("phoenix/assistant/show_thinking_stream")) {
		show_thinking_stream->set_pressed(bool(project_settings->get("phoenix/assistant/show_thinking_stream")));
	}
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
		effective_base_url = _resolve_gateway_base_url();
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

void UltimateAISettingsDialog::set_thinking_stream_enabled(bool p_enabled) {
	if (!show_thinking_stream) {
		return;
	}
	show_thinking_stream->set_pressed(p_enabled);
}

bool UltimateAISettingsDialog::is_thinking_stream_enabled() const {
	return show_thinking_stream ? show_thinking_stream->is_pressed() : false;
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
