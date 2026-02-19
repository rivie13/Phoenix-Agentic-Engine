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
#include "scene/gui/grid_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/link_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tab_container.h"

namespace {
enum ServiceModeIndex {
	SERVICE_MODE_OFFLINE = 0,
	SERVICE_MODE_BYOK = 1,
	SERVICE_MODE_MANAGED_BYOK = 2,
	SERVICE_MODE_MANAGED = 3,
};

String _service_mode_index_to_value(int p_index) {
	switch (p_index) {
		case SERVICE_MODE_OFFLINE:
			return "offline";
		case SERVICE_MODE_BYOK:
			return "byok";
		case SERVICE_MODE_MANAGED_BYOK:
			return "managed_byok";
		default:
			return "managed";
	}
}

int _service_mode_value_to_index(const String &p_value) {
	String value = p_value.strip_edges().to_lower();
	if (value == "offline") {
		return SERVICE_MODE_OFFLINE;
	}
	if (value == "byok") {
		return SERVICE_MODE_BYOK;
	}
	if (value == "managed_byok") {
		return SERVICE_MODE_MANAGED_BYOK;
	}
	return SERVICE_MODE_MANAGED;
}

bool _service_mode_uses_byok_credentials(const String &p_mode) {
	return p_mode == "byok" || p_mode == "managed_byok";
}

bool _allow_env_token_resolution() {
#if defined(DEBUG_ENABLED) || defined(TOOLS_ENABLED)
	return true;
#else
	return false;
#endif
}

bool _allow_static_gateway_token_overrides() {
#if defined(DEBUG_ENABLED) || defined(TOOLS_ENABLED)
	return true;
#else
	return false;
#endif
}

String _default_local_gateway_base_url() {
#ifdef DEBUG_ENABLED
	return "http://localhost:5244";
#else
	return String();
#endif
}

String _default_tier_for_mode(const String &p_mode) {
	if (p_mode == "offline") {
		return "offline";
	}
	if (p_mode == "byok") {
		return "free";
	}
	if (p_mode == "managed_byok") {
		return "pro";
	}
	return "managed";
}

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

	auto append_candidate_dirs = [&](const String &p_dir) {
		String current = p_dir.simplify_path();
		for (int depth = 0; depth < 6; depth++) {
			if (current.is_empty()) {
				break;
			}

			String candidate = current.path_join(".env.local").simplify_path();
			if (!candidate.is_empty() && !candidates.has(candidate)) {
				candidates.push_back(candidate);
			}

			String parent = current.get_base_dir().simplify_path();
			if (parent.is_empty() || parent == current) {
				break;
			}
			current = parent;
		}
	};

	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (project_settings) {
		String project_root = project_settings->globalize_path("res://").simplify_path();
		append_candidate_dirs(project_root);
	}

	Ref<DirAccess> current_dir_access = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (current_dir_access.is_valid()) {
		String current_dir = current_dir_access->get_current_dir().simplify_path();
		append_candidate_dirs(current_dir);
	}

	OS *os = OS::get_singleton();
	if (os) {
		String executable_dir = os->get_executable_path().get_base_dir().simplify_path();
		append_candidate_dirs(executable_dir);
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

String _resolve_gateway_api_token() {
	if (!_allow_env_token_resolution()) {
		return String();
	}

	OS *os = OS::get_singleton();
	if (os) {
		if (os->has_environment("PHOENIX_API_TOKEN")) {
			String value = os->get_environment("PHOENIX_API_TOKEN").strip_edges();
			if (!value.is_empty()) {
				return value;
			}
		}
		if (os->has_environment("PHOENIX_GATEWAY_API_TOKEN")) {
			String value = os->get_environment("PHOENIX_GATEWAY_API_TOKEN").strip_edges();
			if (!value.is_empty()) {
				return value;
			}
		}
	}

	PackedStringArray candidates = _collect_env_file_candidates();
	for (int i = 0; i < candidates.size(); i++) {
		String value = _get_env_file_value(candidates[i], "PHOENIX_API_TOKEN");
		if (!value.is_empty()) {
			return value;
		}
	}

	for (int i = 0; i < candidates.size(); i++) {
		String value = _get_env_file_value(candidates[i], "PHOENIX_GATEWAY_API_TOKEN");
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
	ClassDB::bind_method(D_METHOD("_on_service_mode_selected", "index"), &UltimateAISettingsDialog::_on_service_mode_selected);
	ClassDB::bind_method(D_METHOD("_on_open_url_pressed", "url"), &UltimateAISettingsDialog::_on_open_url_pressed);
}

UltimateAISettingsDialog::UltimateAISettingsDialog() {
	set_title(TTR("Assistant Settings"));
	set_ok_button_text(TTR("Apply"));
	set_min_size(Size2(780, 620));

	VBoxContainer *root = memnew(VBoxContainer);
	root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(root);

	TabContainer *sections = memnew(TabContainer);
	sections->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sections->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(sections);

	auto create_tab_page = [&](const String &p_name) -> VBoxContainer * {
		ScrollContainer *scroll = memnew(ScrollContainer);
		scroll->set_name(p_name);
		scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		sections->add_child(scroll);

		VBoxContainer *page = memnew(VBoxContainer);
		page->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		page->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		page->set_custom_minimum_size(Size2(720, 0));
		scroll->add_child(page);
		return page;
	};

	VBoxContainer *models_page = create_tab_page(TTR("Models"));
	VBoxContainer *service_page = create_tab_page(TTR("Service"));
	VBoxContainer *transport_page = create_tab_page(TTR("Transport"));
	VBoxContainer *mcp_page = create_tab_page(TTR("MCP Tools"));
	VBoxContainer *rules_page = create_tab_page(TTR("Rules"));

	Label *models_label = memnew(Label);
	models_label->set_text(TTR("Available models"));
	models_page->add_child(models_label);

	model_list = memnew(ItemList);
	model_list->set_select_mode(ItemList::SELECT_MULTI);
	model_list->set_allow_reselect(true);
	model_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	models_page->add_child(model_list);

	HBoxContainer *add_row = memnew(HBoxContainer);
	models_page->add_child(add_row);

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
	models_page->add_child(remove_button);

	Label *service_label = memnew(Label);
	service_label->set_text(TTR("Service mode"));
	service_page->add_child(service_label);

	service_mode = memnew(OptionButton);
	service_mode->add_item(TTR("No AI (offline/local only)"));
	service_mode->add_item(TTR("BYOK (single-agent limited)"));
	service_mode->add_item(TTR("Managed BYOK (subscription)"));
	service_mode->add_item(TTR("Managed (subscription)"));
	service_mode->select(SERVICE_MODE_MANAGED);
	service_mode->connect(SceneStringName(item_selected), callable_mp(this, &UltimateAISettingsDialog::_on_service_mode_selected));
	service_page->add_child(service_mode);

	service_mode_help = memnew(Label);
	service_mode_help->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	service_page->add_child(service_mode_help);

	byok_section = memnew(VBoxContainer);
	service_page->add_child(byok_section);

	Label *byok_label = memnew(Label);
	byok_label->set_text(TTR("BYOK API key (stored in memory for this editor session)"));
	byok_section->add_child(byok_label);

	byok_key = memnew(LineEdit);
	byok_key->set_placeholder(TTR("sk-..."));
	byok_key->set_secret(true);
	byok_section->add_child(byok_key);

	Label *byok_warning = memnew(Label);
	byok_warning->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	byok_warning->set_text(TTR("BYOK is intentionally limited compared to managed tiers (single-agent lane, limited tools, no premium managed creative endpoints)."));
	byok_section->add_child(byok_warning);

	service_page->add_child(memnew(HSeparator));

	Label *account_label = memnew(Label);
	account_label->set_text(TTR("Account and setup"));
	service_page->add_child(account_label);

	LinkButton *pricing_link = memnew(LinkButton);
	pricing_link->set_text(TTR("View subscription plans"));
	pricing_link->connect(SceneStringName(pressed), callable_mp(this, &UltimateAISettingsDialog::_on_open_url_pressed).bind(String("https://phoenix-agentic.dev/pricing")));
	service_page->add_child(pricing_link);

	LinkButton *managed_byok_link = memnew(LinkButton);
	managed_byok_link->set_text(TTR("Compare BYOK vs managed tiers"));
	managed_byok_link->connect(SceneStringName(pressed), callable_mp(this, &UltimateAISettingsDialog::_on_open_url_pressed).bind(String("https://phoenix-agentic.dev/docs/service-modes")));
	service_page->add_child(managed_byok_link);

	LinkButton *local_mcp_link = memnew(LinkButton);
	local_mcp_link->set_text(TTR("Local MCP tools setup guide"));
	local_mcp_link->connect(SceneStringName(pressed), callable_mp(this, &UltimateAISettingsDialog::_on_open_url_pressed).bind(String("https://phoenix-agentic.dev/docs/local-mcp")));
	service_page->add_child(local_mcp_link);

	Label *transport_label = memnew(Label);
	transport_label->set_text(TTR("Gateway transport"));
	transport_page->add_child(transport_label);

	GridContainer *transport_grid = memnew(GridContainer);
	transport_grid->set_columns(2);
	transport_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	transport_page->add_child(transport_grid);

	Label *timeout_label = memnew(Label);
	timeout_label->set_text(TTR("Timeout (ms)"));
	transport_grid->add_child(timeout_label);

	timeout_ms = memnew(SpinBox);
	timeout_ms->set_min(1000);
	timeout_ms->set_max(120000);
	timeout_ms->set_step(500);
	timeout_ms->set_value(15000);
	timeout_ms->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	transport_grid->add_child(timeout_ms);

	Label *retry_label = memnew(Label);
	retry_label->set_text(TTR("Retry"));
	transport_grid->add_child(retry_label);

	retry_count = memnew(SpinBox);
	retry_count->set_min(0);
	retry_count->set_max(5);
	retry_count->set_step(1);
	retry_count->set_value(1);
	retry_count->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	transport_grid->add_child(retry_count);

	Label *mcp_label = memnew(Label);
	mcp_label->set_text(TTR("MCP tool setup"));
	mcp_page->add_child(mcp_label);

	Label *mcp_help = memnew(Label);
	mcp_help->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	mcp_help->set_text(TTR("Configure local MCP connectivity and discovery for editor tool integrations."));
	mcp_page->add_child(mcp_help);

	mcp_enabled = memnew(CheckBox);
	mcp_enabled->set_text(TTR("Enable MCP tools"));
	mcp_enabled->set_pressed(true);
	mcp_page->add_child(mcp_enabled);

	Label *base_tools_label = memnew(Label);
	base_tools_label->set_text(TTR("Base tool integrations"));
	mcp_page->add_child(base_tools_label);

	tool_godot_mcp_docs_enabled = memnew(CheckBox);
	tool_godot_mcp_docs_enabled->set_text(TTR("Enable godot-mcp-docs"));
	tool_godot_mcp_docs_enabled->set_pressed(true);
	mcp_page->add_child(tool_godot_mcp_docs_enabled);

	tool_godot_mcp_enabled = memnew(CheckBox);
	tool_godot_mcp_enabled->set_text(TTR("Enable godot-mcp"));
	tool_godot_mcp_enabled->set_pressed(true);
	mcp_page->add_child(tool_godot_mcp_enabled);

	tool_godot_copilot_enabled = memnew(CheckBox);
	tool_godot_copilot_enabled->set_text(TTR("Enable godot-copilot"));
	tool_godot_copilot_enabled->set_pressed(true);
	mcp_page->add_child(tool_godot_copilot_enabled);

	tool_autonomous_primitives_enabled = memnew(CheckBox);
	tool_autonomous_primitives_enabled->set_text(TTR("Enable autonomous-agent primitives"));
	tool_autonomous_primitives_enabled->set_pressed(true);
	mcp_page->add_child(tool_autonomous_primitives_enabled);

	Label *mcp_transport_label = memnew(Label);
	mcp_transport_label->set_text(TTR("Transport"));
	mcp_page->add_child(mcp_transport_label);

	mcp_transport = memnew(OptionButton);
	mcp_transport->add_item(TTR("stdio (local)"));
	mcp_transport->add_item(TTR("http (bridge)"));
	mcp_transport->select(0);
	mcp_page->add_child(mcp_transport);

	mcp_auto_discover = memnew(CheckBox);
	mcp_auto_discover->set_text(TTR("Auto-discover project MCP servers"));
	mcp_auto_discover->set_pressed(true);
	mcp_page->add_child(mcp_auto_discover);

	mcp_require_approvals = memnew(CheckBox);
	mcp_require_approvals->set_text(TTR("Require approvals for MCP write actions"));
	mcp_require_approvals->set_pressed(true);
	mcp_page->add_child(mcp_require_approvals);

	Label *mcp_path_label = memnew(Label);
	mcp_path_label->set_text(TTR("MCP config path (optional)"));
	mcp_page->add_child(mcp_path_label);

	mcp_config_path = memnew(LineEdit);
	mcp_config_path->set_placeholder(TTR("res://.phoenix/mcp.json"));
	mcp_page->add_child(mcp_config_path);

	rules_page->add_child(memnew(HSeparator));

	Label *rules_label = memnew(Label);
	rules_label->set_text(TTR("Rules"));
	rules_page->add_child(rules_label);

	allow_background_agents = memnew(CheckBox);
	allow_background_agents->set_text(TTR("Allow background agents"));
	allow_background_agents->set_pressed(true);
	rules_page->add_child(allow_background_agents);

	auto_approve_reads = memnew(CheckBox);
	auto_approve_reads->set_text(TTR("Auto-approve read tools"));
	auto_approve_reads->set_pressed(true);
	rules_page->add_child(auto_approve_reads);

	require_approvals = memnew(CheckBox);
	require_approvals->set_text(TTR("Require approvals for write tools"));
	require_approvals->set_pressed(true);
	rules_page->add_child(require_approvals);

	require_signed_commands = memnew(CheckBox);
	require_signed_commands->set_text(TTR("Require signatures for executable commands"));
	require_signed_commands->set_pressed(true);
	rules_page->add_child(require_signed_commands);

	show_thinking_stream = memnew(CheckBox);
	show_thinking_stream->set_text(TTR("Show thinking stream (when available)"));
	show_thinking_stream->set_pressed(false);
	rules_page->add_child(show_thinking_stream);

	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (project_settings && project_settings->has_setting("phoenix/assistant/show_thinking_stream")) {
		show_thinking_stream->set_pressed(bool(project_settings->get("phoenix/assistant/show_thinking_stream")));
	}

	_on_service_mode_selected(service_mode ? service_mode->get_selected() : SERVICE_MODE_MANAGED);
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
	String service_mode_value = String(p_config.get("service_mode", p_config.get("auth_mode", String("managed")))).to_lower();
	if (service_mode) {
		service_mode->select(_service_mode_value_to_index(service_mode_value));
	}

	persisted_base_url = String(p_config.get("base_url", String())).strip_edges();
	// Security: do not store redaction sentinels — they are placeholders only.
	String incoming_token = String(p_config.get("token", String())).strip_edges();
	if (incoming_token != "<configured>") {
		persisted_token = incoming_token;
	}
	String incoming_hook = String(p_config.get("token_hook", String())).strip_edges();
	if (incoming_hook != "<configured>") {
		persisted_token_hook = incoming_hook;
	}
	persisted_actor_id = String(p_config.get("actor_id", String())).strip_edges();
	persisted_tier = String(p_config.get("tier", String())).strip_edges();

	if (byok_key) {
		byok_key->clear();
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
	if (p_config.has("allow_background_agents") && allow_background_agents) {
		allow_background_agents->set_pressed(bool(p_config["allow_background_agents"]));
	}
	if (p_config.has("auto_approve_reads") && auto_approve_reads) {
		auto_approve_reads->set_pressed(bool(p_config["auto_approve_reads"]));
	}
	if (p_config.has("require_approvals") && require_approvals) {
		require_approvals->set_pressed(bool(p_config["require_approvals"]));
	}
	if (p_config.has("mcp_enabled") && mcp_enabled) {
		mcp_enabled->set_pressed(bool(p_config["mcp_enabled"]));
	}
	if (p_config.has("tool_godot_mcp_docs_enabled") && tool_godot_mcp_docs_enabled) {
		tool_godot_mcp_docs_enabled->set_pressed(bool(p_config["tool_godot_mcp_docs_enabled"]));
	}
	if (p_config.has("tool_godot_mcp_enabled") && tool_godot_mcp_enabled) {
		tool_godot_mcp_enabled->set_pressed(bool(p_config["tool_godot_mcp_enabled"]));
	}
	if (p_config.has("tool_godot_copilot_enabled") && tool_godot_copilot_enabled) {
		tool_godot_copilot_enabled->set_pressed(bool(p_config["tool_godot_copilot_enabled"]));
	}
	if (p_config.has("tool_autonomous_primitives_enabled") && tool_autonomous_primitives_enabled) {
		tool_autonomous_primitives_enabled->set_pressed(bool(p_config["tool_autonomous_primitives_enabled"]));
	}
	if (mcp_transport) {
		String transport_value = String(p_config.get("mcp_transport", String("stdio"))).to_lower();
		mcp_transport->select(transport_value == "http" ? 1 : 0);
	}
	if (p_config.has("mcp_auto_discover") && mcp_auto_discover) {
		mcp_auto_discover->set_pressed(bool(p_config["mcp_auto_discover"]));
	}
	if (p_config.has("mcp_require_approvals") && mcp_require_approvals) {
		mcp_require_approvals->set_pressed(bool(p_config["mcp_require_approvals"]));
	}
	if (mcp_config_path) {
		mcp_config_path->set_text(String(p_config.get("mcp_config_path", String())).strip_edges());
	}

	_on_service_mode_selected(service_mode ? service_mode->get_selected() : SERVICE_MODE_MANAGED);
}

Dictionary UltimateAISettingsDialog::get_runtime_config() const {
	Dictionary config;

	String service_mode_value = "managed";
	if (service_mode) {
		service_mode_value = _service_mode_index_to_value(service_mode->get_selected());
	}

	String effective_base_url = persisted_base_url;
	if (service_mode_value != "offline" && effective_base_url.is_empty()) {
		effective_base_url = _resolve_gateway_base_url();
	}
	if (service_mode_value != "offline" && effective_base_url.is_empty()) {
		effective_base_url = _default_local_gateway_base_url();
	}
	if (service_mode_value == "offline") {
		effective_base_url = String();
	}

	String token_value = byok_key ? byok_key->get_text().strip_edges() : String();
	if (token_value.is_empty()) {
		token_value = persisted_token;
	}
	if (_allow_env_token_resolution() && token_value.is_empty() && service_mode_value != "offline") {
		token_value = _resolve_gateway_api_token();
	}

	String token_hook_value = persisted_token_hook;
	if (_allow_env_token_resolution() && service_mode_value != "offline" && token_value.is_empty() && token_hook_value.is_empty()) {
		token_hook_value = "env:PHOENIX_API_TOKEN";
	}
	if (!_allow_static_gateway_token_overrides()) {
		token_value = String();
		token_hook_value = String();
	}
	if (service_mode_value == "offline") {
		token_value = String();
		token_hook_value = String();
	}

	String actor_id_value = persisted_actor_id;
	if (actor_id_value.is_empty()) {
		actor_id_value = "editor-user";
	}

	String tier_value = persisted_tier;
	if (tier_value.is_empty()) {
		tier_value = _default_tier_for_mode(service_mode_value);
	}

	config["service_mode"] = service_mode_value;
	config["auth_mode"] = service_mode_value;
	config["base_url"] = effective_base_url;
	config["token"] = token_value;
	config["token_hook"] = token_hook_value;
	config["actor_id"] = actor_id_value;
	config["tier"] = tier_value;
	config["timeout_ms"] = timeout_ms ? int(timeout_ms->get_value()) : 15000;
	config["retry_count"] = retry_count ? int(retry_count->get_value()) : 1;
	config["require_signed_commands"] = require_signed_commands ? require_signed_commands->is_pressed() : true;
	config["allow_background_agents"] = allow_background_agents ? allow_background_agents->is_pressed() : true;
	config["auto_approve_reads"] = auto_approve_reads ? auto_approve_reads->is_pressed() : true;
	config["require_approvals"] = require_approvals ? require_approvals->is_pressed() : true;
	config["mcp_enabled"] = mcp_enabled ? mcp_enabled->is_pressed() : true;
	config["tool_godot_mcp_docs_enabled"] = tool_godot_mcp_docs_enabled ? tool_godot_mcp_docs_enabled->is_pressed() : true;
	config["tool_godot_mcp_enabled"] = tool_godot_mcp_enabled ? tool_godot_mcp_enabled->is_pressed() : true;
	config["tool_godot_copilot_enabled"] = tool_godot_copilot_enabled ? tool_godot_copilot_enabled->is_pressed() : true;
	config["tool_autonomous_primitives_enabled"] = tool_autonomous_primitives_enabled ? tool_autonomous_primitives_enabled->is_pressed() : true;
	config["mcp_transport"] = (mcp_transport && mcp_transport->get_selected() == 1) ? String("http") : String("stdio");
	config["mcp_auto_discover"] = mcp_auto_discover ? mcp_auto_discover->is_pressed() : true;
	config["mcp_require_approvals"] = mcp_require_approvals ? mcp_require_approvals->is_pressed() : true;
	config["mcp_config_path"] = mcp_config_path ? mcp_config_path->get_text().strip_edges() : String();

	PackedStringArray allowlist;
	allowlist.push_back("create_file");
	allowlist.push_back("modify_text");
	allowlist.push_back("create_node");
	allowlist.push_back("chat_message");
	allowlist.push_back("open_docs_query");
	allowlist.push_back("open_docs_file");
	allowlist.push_back("open_docs_url");
	config["command_allowlist"] = allowlist;

	return config;
}

void UltimateAISettingsDialog::_on_service_mode_selected(int p_index) {
	String mode_value = _service_mode_index_to_value(p_index);
	bool allow_byok_entry = _service_mode_uses_byok_credentials(mode_value);

	if (byok_key) {
		byok_key->set_editable(allow_byok_entry);
		if (!allow_byok_entry) {
			byok_key->clear();
		}
	}
	if (byok_section) {
		byok_section->set_visible(allow_byok_entry);
	}

	if (!service_mode_help) {
		return;
	}

	if (mode_value == "offline") {
		service_mode_help->set_text(TTR("Offline mode disables backend orchestration. Use manual/local editor workflows only."));
		return;
	}
	if (mode_value == "byok") {
		service_mode_help->set_text(TTR("BYOK mode is lower capability than managed tiers: single-agent lane, limited tool profile, and no premium managed orchestration."));
		return;
	}
	if (mode_value == "managed_byok") {
		service_mode_help->set_text(TTR("Managed BYOK mode uses your key with managed orchestration and requires an active Phoenix subscription."));
		return;
	}
	service_mode_help->set_text(TTR("Managed mode uses Phoenix-managed credentials and requires an active Phoenix subscription."));
}

void UltimateAISettingsDialog::_on_open_url_pressed(const String &p_url) {
	String url = p_url.strip_edges();
	if (url.is_empty()) {
		return;
	}
	OS::get_singleton()->shell_open(url);
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
