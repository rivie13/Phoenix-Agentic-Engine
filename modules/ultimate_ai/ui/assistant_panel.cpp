/**************************************************************************/
/*  assistant_panel.cpp                                                   */
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

#include "assistant_panel.h"

#include "assistant_settings_dialog.h"

#include "core/backend_contract_adapter.h"
#include "core/frontend_runtime_adapter.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/object/worker_thread_pool.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"
#include "core/variant/variant_utility.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/script/script_editor_plugin.h"
#include "modules/websocket/websocket_peer.h"
#include "scene/animation/tween.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "servers/display/display_server.h"

Vector<UltimateAssistantPanel *> UltimateAssistantPanel::s_instances;
Vector<UltimateAssistantPanel::SharedChatTabState> UltimateAssistantPanel::s_shared_tabs;
Vector<UltimateAssistantPanel::ArchivedSession> UltimateAssistantPanel::s_shared_archived_sessions;
PackedStringArray UltimateAssistantPanel::s_shared_models;
Dictionary UltimateAssistantPanel::s_shared_pixelpen_snapshot;
Array UltimateAssistantPanel::s_shared_pixelpen_layers;
int UltimateAssistantPanel::s_shared_tab_counter = 0;
int UltimateAssistantPanel::s_shared_current_tab = 0;
bool UltimateAssistantPanel::s_shared_initialized = false;

namespace {
const char *const PIXELPEN_SNAPSHOT_KIND = "pixelpen_snapshot";
const char *const PIXELPEN_LAYER_KIND = "pixelpen_layer";
const char *const MODE_ASK = "ask";
const char *const MODE_PLAN = "plan";
const char *const MODE_AGENT = "agent";

struct ToolOptionDescriptor {
	const char *id;
	const char *label;
	const char *runtime_flag_key;
};

const ToolOptionDescriptor TOOL_OPTION_DESCRIPTORS[] = {
	{ "godot_mcp_docs", "godot-mcp-docs", "tool_godot_mcp_docs_enabled" },
	{ "godot_mcp", "godot-mcp", "tool_godot_mcp_enabled" },
	{ "godot_copilot", "godot-copilot", "tool_godot_copilot_enabled" },
	{ "autonomous_agent_primitives", "autonomous-agent primitives", "tool_autonomous_primitives_enabled" },
};
const int TOOL_OPTION_COUNT = 4;

const int TASK_STATUS_LIVE_POLL_INTERVAL_MSEC = 120;
const int TASK_STATUS_LIVE_POLL_INITIAL_DELAY_MSEC = 25;
const int COMMAND_STREAM_TICK_MSEC = 20;
const int COMMAND_STREAM_CHUNK_CHARS = 24;
const int LOADING_INDICATOR_TICK_MSEC = 120;
const char *const LOADING_SPINNER_FRAMES[] = {
	"|",
	"/",
	"-",
	"\\",
};
const int LOADING_SPINNER_FRAME_COUNT = 4;

bool _is_ascii_alpha(char32_t p_char) {
	return (p_char >= 'A' && p_char <= 'Z') || (p_char >= 'a' && p_char <= 'z');
}

bool _is_ascii_digit(char32_t p_char) {
	return p_char >= '0' && p_char <= '9';
}

bool _is_ascii_identifier_char(char32_t p_char) {
	return _is_ascii_alpha(p_char) || _is_ascii_digit(p_char) || p_char == '_';
}

String _sanitize_docs_identifier(const String &p_value) {
	String value = p_value.strip_edges();
	if (value.is_empty()) {
		return String();
	}

	String sanitized;
	for (int i = 0; i < value.length(); i++) {
		char32_t character = value[i];
		if (_is_ascii_identifier_char(character)) {
			sanitized += String::chr(character);
		}
	}

	if (sanitized.length() < 3 || (!_is_ascii_alpha(sanitized[0]) && sanitized[0] != '_')) {
		return String();
	}

	return sanitized;
}

void _append_unique_docs_candidate(const String &p_candidate, Vector<String> &r_candidates) {
	if (p_candidate.is_empty()) {
		return;
	}

	for (int i = 0; i < r_candidates.size(); i++) {
		if (r_candidates[i].nocasecmp_to(p_candidate) == 0) {
			return;
		}
	}

	r_candidates.push_back(p_candidate);
}

void _extract_docs_candidates_from_text(const String &p_text, Vector<String> &r_candidates) {
	String token;
	auto flush_token = [&]() {
		String sanitized = _sanitize_docs_identifier(token);
		if (!sanitized.is_empty()) {
			_append_unique_docs_candidate(sanitized, r_candidates);
		}
		token.clear();
	};

	for (int i = 0; i < p_text.length(); i++) {
		char32_t character = p_text[i];
		if (_is_ascii_identifier_char(character)) {
			token += String::chr(character);
		} else {
			flush_token();
		}
	}

	flush_token();
}

bool _is_allowed_help_topic(const String &p_topic) {
	String topic = p_topic.strip_edges();
	if (topic.is_empty()) {
		return false;
	}

	bool valid_prefix = topic.begins_with("class:") || topic.begins_with("class_name:") || topic.begins_with("class_method:") || topic.begins_with("class_signal:") || topic.begins_with("class_property:") || topic.begins_with("class_theme_item:");
	if (!valid_prefix) {
		return false;
	}

	for (int i = 0; i < topic.length(); i++) {
		char32_t character = topic[i];
		if (_is_ascii_identifier_char(character) || character == ':' || character == '@') {
			continue;
		}
		return false;
	}

	return true;
}

bool _is_supported_command_path(const String &p_path) {
	String path = p_path.strip_edges();
	if (path.is_empty()) {
		return false;
	}
	if (!path.begins_with("res://") && !path.begins_with("user://")) {
		return false;
	}
	if (path.find("..") >= 0) {
		return false;
	}
	return true;
}

bool _decode_base64_bytes(const String &p_base64, Vector<uint8_t> &r_bytes) {
	r_bytes.clear();
	if (p_base64.is_empty()) {
		return false;
	}

	CharString cstr = p_base64.ascii();
	const int str_len = p_base64.length();
	Vector<uint8_t> bytes;
	bytes.resize(str_len / 4 * 3 + 1);

	size_t decoded_len = 0;
	uint8_t *write = bytes.ptrw();
	Error err = CryptoCore::b64_decode(write, bytes.size(), &decoded_len, (const uint8_t *)cstr.get_data(), str_len);
	if (err != OK) {
		return false;
	}

	bytes.resize(decoded_len);
	r_bytes = bytes;
	return true;
}

Node *_find_node_by_name_recursive(Node *p_root, const String &p_name) {
	if (!p_root) {
		return nullptr;
	}

	if (p_root->get_name() == p_name) {
		return p_root;
	}

	const int child_count = p_root->get_child_count();
	for (int i = 0; i < child_count; i++) {
		Node *child = p_root->get_child(i);
		Node *match = _find_node_by_name_recursive(child, p_name);
		if (match) {
			return match;
		}
	}

	return nullptr;
}

Node *_resolve_command_parent_node(Node *p_edited_scene_root, const String &p_parent) {
	if (!p_edited_scene_root) {
		return nullptr;
	}

	String parent = p_parent.strip_edges();
	if (parent.is_empty() || parent == ".") {
		return p_edited_scene_root;
	}

	if (parent == p_edited_scene_root->get_name()) {
		return p_edited_scene_root;
	}

	String normalized_parent = parent;
	if (normalized_parent.begins_with("/")) {
		normalized_parent = normalized_parent.substr(1, normalized_parent.length() - 1).strip_edges();
	}

	String root_prefix = String(p_edited_scene_root->get_name()) + "/";
	if (normalized_parent.begins_with(root_prefix)) {
		normalized_parent = normalized_parent.substr(root_prefix.length(), normalized_parent.length() - root_prefix.length()).strip_edges();
	}

	if (normalized_parent.is_empty() || normalized_parent == p_edited_scene_root->get_name()) {
		return p_edited_scene_root;
	}

	Node *by_path = p_edited_scene_root->get_node_or_null(NodePath(normalized_parent));
	if (by_path) {
		return by_path;
	}

	return _find_node_by_name_recursive(p_edited_scene_root, normalized_parent);
}

bool _is_truthy_flag(const String &p_value) {
	String value = p_value.strip_edges().to_lower();
	return value == "1" || value == "true" || value == "yes" || value == "on";
}

bool _is_falsy_flag(const String &p_value) {
	String value = p_value.strip_edges().to_lower();
	return value == "0" || value == "false" || value == "no" || value == "off";
}

String _tool_label_for_id(const String &p_tool_id) {
	for (int i = 0; i < TOOL_OPTION_COUNT; i++) {
		if (p_tool_id == TOOL_OPTION_DESCRIPTORS[i].id) {
			return TOOL_OPTION_DESCRIPTORS[i].label;
		}
	}
	return p_tool_id;
}

bool _is_thinking_stream_enabled() {
	OS *os = OS::get_singleton();
	if (os) {
		String env_value = os->get_environment("PHOENIX_ENABLE_THINKING_STREAM");
		if (_is_truthy_flag(env_value)) {
			return true;
		}
		if (_is_falsy_flag(env_value)) {
			return false;
		}
	}

	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (!project_settings || !project_settings->has_setting("phoenix/assistant/show_thinking_stream")) {
		return false;
	}

	return bool(project_settings->get("phoenix/assistant/show_thinking_stream"));
}

String _append_access_token_to_url(const String &p_url, const String &p_access_token) {
	String url = p_url.strip_edges();
	if (url.is_empty()) {
		return url;
	}

	String access_token = p_access_token.strip_edges();
	if (access_token.is_empty()) {
		return url;
	}

	String separator = url.contains("?") ? "&" : "?";
	return url + separator + "access_token=" + access_token.uri_encode();
}

void _collect_realtime_event_payloads(const Variant &p_payload, Array &r_events, int p_depth = 0) {
	if (p_depth > 5 || p_payload.get_type() == Variant::NIL) {
		return;
	}

	if (p_payload.get_type() == Variant::ARRAY) {
		Array values = p_payload;
		for (int i = 0; i < values.size(); i++) {
			_collect_realtime_event_payloads(values[i], r_events, p_depth + 1);
		}
		return;
	}

	if (p_payload.get_type() == Variant::STRING) {
		String text = String(p_payload).strip_edges();
		if (!text.begins_with("{") && !text.begins_with("[")) {
			return;
		}

		Variant parsed = JSON::parse_string(text);
		if (parsed.get_type() == Variant::NIL && text != "null") {
			return;
		}
		_collect_realtime_event_payloads(parsed, r_events, p_depth + 1);
		return;
	}

	if (p_payload.get_type() != Variant::DICTIONARY) {
		return;
	}

	Dictionary dictionary = p_payload;
	String schema_version = String(dictionary.get("schema_version", String())).strip_edges();
	String event_name = String(dictionary.get("event", String())).strip_edges();
	if (!schema_version.is_empty() && !event_name.is_empty()) {
		r_events.push_back(dictionary);
		return;
	}

	if (dictionary.has("data")) {
		_collect_realtime_event_payloads(dictionary["data"], r_events, p_depth + 1);
	}
	if (dictionary.has("message")) {
		_collect_realtime_event_payloads(dictionary["message"], r_events, p_depth + 1);
	}
}

String _escape_bbcode_text(const String &p_text) {
	String escaped = p_text;
	escaped = escaped.replace("[", "[lb]");
	escaped = escaped.replace("]", "[rb]");
	return escaped;
}

bool _is_numbered_list_item(const String &p_line, int &r_content_start) {
	r_content_start = -1;
	if (p_line.is_empty()) {
		return false;
	}

	int index = 0;
	while (index < p_line.length() && p_line[index] >= '0' && p_line[index] <= '9') {
		index++;
	}

	if (index == 0) {
		return false;
	}
	if (index + 1 >= p_line.length()) {
		return false;
	}
	if (p_line[index] != '.' || p_line[index + 1] != ' ') {
		return false;
	}

	r_content_start = index + 2;
	return true;
}

String _markdown_inline_to_bbcode(const String &p_text) {
	String output;
	bool bold_open = false;
	bool italic_open = false;

	for (int i = 0; i < p_text.length(); i++) {
		char32_t chr = p_text[i];

		if (chr == '\\' && i + 1 < p_text.length()) {
			output += _escape_bbcode_text(String::chr(p_text[i + 1]));
			i++;
			continue;
		}

		if (chr == '`') {
			int close_tick = p_text.find_char('`', i + 1);
			if (close_tick > i + 1) {
				String code = p_text.substr(i + 1, close_tick - i - 1);
				output += "[code]" + _escape_bbcode_text(code) + "[/code]";
				i = close_tick;
				continue;
			}
		}

		if (chr == '[') {
			int close_bracket = p_text.find_char(']', i + 1);
			if (close_bracket > i + 1 && close_bracket + 1 < p_text.length() && p_text[close_bracket + 1] == '(') {
				int close_paren = p_text.find_char(')', close_bracket + 2);
				if (close_paren > close_bracket + 2) {
					String label = p_text.substr(i + 1, close_bracket - i - 1);
					String url = p_text.substr(close_bracket + 2, close_paren - close_bracket - 2).strip_edges();
					if (url.begins_with("http://") || url.begins_with("https://")) {
						output += "[url=" + _escape_bbcode_text(url) + "]" + _markdown_inline_to_bbcode(label) + "[/url]";
						i = close_paren;
						continue;
					}
				}
			}
		}

		if (chr == '*' && i + 1 < p_text.length() && p_text[i + 1] == '*') {
			output += bold_open ? "[/b]" : "[b]";
			bold_open = !bold_open;
			i++;
			continue;
		}

		if (chr == '*') {
			output += italic_open ? "[/i]" : "[i]";
			italic_open = !italic_open;
			continue;
		}

		output += _escape_bbcode_text(String::chr(chr));
	}

	if (bold_open) {
		output += "[/b]";
	}
	if (italic_open) {
		output += "[/i]";
	}

	return output;
}

String _markdown_to_bbcode(const String &p_text) {
	String normalized = p_text.replace("\r\n", "\n").replace("\r", "\n");
	PackedStringArray lines = normalized.split("\n", true);

	String output;
	bool in_code_block = false;
	String code_block;

	for (int i = 0; i < lines.size(); i++) {
		String line = lines[i];
		String trimmed = line.strip_edges();

		if (trimmed.begins_with("```")) {
			if (in_code_block) {
				output += "[code]" + _escape_bbcode_text(code_block) + "[/code]\n";
				code_block.clear();
				in_code_block = false;
			} else {
				in_code_block = true;
			}
			continue;
		}

		if (in_code_block) {
			code_block += line;
			if (i < lines.size() - 1) {
				code_block += "\n";
			}
			continue;
		}

		if (trimmed.is_empty()) {
			output += "\n";
			continue;
		}

		if (trimmed.begins_with("### ")) {
			output += "[b]" + _markdown_inline_to_bbcode(trimmed.substr(4)) + "[/b]\n";
			continue;
		}
		if (trimmed.begins_with("## ")) {
			output += "[b]" + _markdown_inline_to_bbcode(trimmed.substr(3)) + "[/b]\n";
			continue;
		}
		if (trimmed.begins_with("# ")) {
			output += "[b]" + _markdown_inline_to_bbcode(trimmed.substr(2)) + "[/b]\n";
			continue;
		}

		if (trimmed.begins_with("- ") || trimmed.begins_with("* ")) {
			output += String::chr(0x2022) + " " + _markdown_inline_to_bbcode(trimmed.substr(2)) + "\n";
			continue;
		}

		int list_content_start = -1;
		if (_is_numbered_list_item(trimmed, list_content_start)) {
			output += String::chr(0x2022) + " " + _markdown_inline_to_bbcode(trimmed.substr(list_content_start)) + "\n";
			continue;
		}

		if (trimmed.begins_with("> ")) {
			output += "[i]" + _markdown_inline_to_bbcode(trimmed.substr(2)) + "[/i]\n";
			continue;
		}

		output += _markdown_inline_to_bbcode(line) + "\n";
	}

	if (in_code_block) {
		output += "[code]" + _escape_bbcode_text(code_block) + "[/code]\n";
	}

	return output.strip_edges(false, true);
}

bool _has_nonempty_string_field(const Dictionary &p_dict, const char *p_key) {
	if (!p_dict.has(p_key)) {
		return false;
	}
	Variant value = p_dict[p_key];
	if (value.get_type() != Variant::STRING) {
		return false;
	}
	return !String(value).strip_edges().is_empty();
}

bool _is_valid_task_request_accepted_payload(const Dictionary &p_body) {
	if (!_has_nonempty_string_field(p_body, "schema_version") || String(p_body["schema_version"]) != "v1") {
		return false;
	}
	if (!_has_nonempty_string_field(p_body, "event") || String(p_body["event"]) != "task_queued_ack") {
		return false;
	}
	if (!p_body.has("accepted") || p_body["accepted"].get_type() != Variant::BOOL || !bool(p_body["accepted"])) {
		return false;
	}
	if (!_has_nonempty_string_field(p_body, "session_id") || !_has_nonempty_string_field(p_body, "task_id")) {
		return false;
	}
	if (!_has_nonempty_string_field(p_body, "plan_id") || !_has_nonempty_string_field(p_body, "job_id")) {
		return false;
	}
	if (!_has_nonempty_string_field(p_body, "status") || String(p_body["status"]).to_lower() != "queued") {
		return false;
	}
	if (!_has_nonempty_string_field(p_body, "tier")) {
		return false;
	}
	return true;
}

bool _is_known_task_status(const String &p_status) {
	return p_status == "queued" || p_status == "planning" || p_status == "awaiting_approval" ||
			p_status == "approved" || p_status == "executing" || p_status == "done" || p_status == "error";
}

bool _is_valid_task_status_payload(const Dictionary &p_body, const String &p_expected_plan_id) {
	if (!_has_nonempty_string_field(p_body, "schema_version") || String(p_body["schema_version"]) != "v1") {
		return false;
	}
	if (!_has_nonempty_string_field(p_body, "plan_id") || !_has_nonempty_string_field(p_body, "job_id") || !_has_nonempty_string_field(p_body, "session_id")) {
		return false;
	}
	if (!_has_nonempty_string_field(p_body, "tier") || !_has_nonempty_string_field(p_body, "updated_at")) {
		return false;
	}
	String plan_id = String(p_body["plan_id"]);
	if (!p_expected_plan_id.is_empty() && plan_id != p_expected_plan_id) {
		return false;
	}
	if (!_has_nonempty_string_field(p_body, "status") || !_is_known_task_status(String(p_body["status"]).to_lower())) {
		return false;
	}
	return true;
}

String _readable_mode_from_selector(OptionButton *p_selector) {
	if (!p_selector) {
		return MODE_ASK;
	}
	int selected = p_selector->get_selected();
	if (selected == 1) {
		return MODE_PLAN;
	}
	if (selected == 2) {
		return MODE_AGENT;
	}
	return MODE_ASK;
}

String _sanitize_reviewer_id(const String &p_value) {
	String trimmed = p_value.strip_edges();
	if (trimmed.is_empty()) {
		return String();
	}

	String sanitized;
	for (int i = 0; i < trimmed.length(); i++) {
		char32_t chr = trimmed[i];
		bool is_ascii_letter = (chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z');
		bool is_digit = chr >= '0' && chr <= '9';
		bool is_safe_punctuation = chr == '-' || chr == '_' || chr == '.';
		if (is_ascii_letter || is_digit || is_safe_punctuation) {
			sanitized += String::chr(chr);
		}
	}

	if (sanitized.length() > 64) {
		sanitized = sanitized.substr(0, 64);
	}

	return sanitized;
}
String _now_iso8601_utc() {
	return Time::get_singleton()->get_datetime_string_from_system(true, false) + "Z";
}

String _id_with_prefix(const String &p_prefix) {
	uint64_t unix_time = (uint64_t)Time::get_singleton()->get_unix_time_from_system();
	uint64_t ticks = OS::get_singleton()->get_ticks_usec();
	return p_prefix + "-" + itos((int64_t)unix_time) + "-" + itos((int64_t)ticks);
}

int _suffix_prefix_overlap_length(const String &p_existing, const String &p_incoming) {
	if (p_existing.is_empty() || p_incoming.is_empty()) {
		return 0;
	}

	int max_overlap = MIN(p_existing.length(), p_incoming.length());
	for (int overlap = max_overlap; overlap > 0; overlap--) {
		bool matches = true;
		for (int i = 0; i < overlap; i++) {
			if (p_existing[p_existing.length() - overlap + i] != p_incoming[i]) {
				matches = false;
				break;
			}
		}
		if (matches) {
			return overlap;
		}
	}

	return 0;
}
} //namespace

void UltimateAssistantPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_new_tab_pressed"), &UltimateAssistantPanel::_on_new_tab_pressed);
	ClassDB::bind_method(D_METHOD("_on_tab_close_requested", "tab_index"), &UltimateAssistantPanel::_on_tab_close_requested);
	ClassDB::bind_method(D_METHOD("_on_send_pressed", "tab_id"), &UltimateAssistantPanel::_on_send_pressed);
	ClassDB::bind_method(D_METHOD("_on_copy_pressed", "tab_id"), &UltimateAssistantPanel::_on_copy_pressed);
	ClassDB::bind_method(D_METHOD("_on_input_submitted", "text", "tab_id"), &UltimateAssistantPanel::_on_input_submitted);
	ClassDB::bind_method(D_METHOD("_on_interrupt_pressed", "tab_id"), &UltimateAssistantPanel::_on_interrupt_pressed);
	ClassDB::bind_method(D_METHOD("_on_steer_pressed", "tab_id"), &UltimateAssistantPanel::_on_steer_pressed);
	ClassDB::bind_method(D_METHOD("_on_settings_pressed"), &UltimateAssistantPanel::_on_settings_pressed);
	ClassDB::bind_method(D_METHOD("_on_settings_confirmed"), &UltimateAssistantPanel::_on_settings_confirmed);
	ClassDB::bind_method(D_METHOD("_on_tab_setting_changed", "selected_index", "tab_id"), &UltimateAssistantPanel::_on_tab_setting_changed);
	ClassDB::bind_method(D_METHOD("_on_tool_selection_changed", "index", "selected", "tab_id"), &UltimateAssistantPanel::_on_tool_selection_changed);
	ClassDB::bind_method(D_METHOD("_on_context_add_pressed", "tab_id"), &UltimateAssistantPanel::_on_context_add_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_remove_pressed", "tab_id"), &UltimateAssistantPanel::_on_context_remove_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_select_add_pressed"), &UltimateAssistantPanel::_on_context_select_add_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_select_remove_pressed"), &UltimateAssistantPanel::_on_context_select_remove_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_note_add_pressed"), &UltimateAssistantPanel::_on_context_note_add_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_search_changed", "text"), &UltimateAssistantPanel::_on_context_search_changed);
	ClassDB::bind_method(D_METHOD("_on_context_toggle_toggled", "pressed", "tab_id"), &UltimateAssistantPanel::_on_context_toggle_toggled);
	ClassDB::bind_method(D_METHOD("_on_context_dialog_confirmed"), &UltimateAssistantPanel::_on_context_dialog_confirmed);
	ClassDB::bind_method(D_METHOD("_on_previous_reopen_pressed"), &UltimateAssistantPanel::_on_previous_reopen_pressed);
	ClassDB::bind_method(D_METHOD("_on_previous_session_activated", "index"), &UltimateAssistantPanel::_on_previous_session_activated);
	ClassDB::bind_method(D_METHOD("_on_session_name_changed", "text", "tab_id"), &UltimateAssistantPanel::_on_session_name_changed);
	ClassDB::bind_method(D_METHOD("_on_approve_pressed", "tab_id"), &UltimateAssistantPanel::_on_approve_pressed);
	ClassDB::bind_method(D_METHOD("_on_reject_pressed", "tab_id"), &UltimateAssistantPanel::_on_reject_pressed);
	ClassDB::bind_method(D_METHOD("_on_resync_pressed", "tab_id"), &UltimateAssistantPanel::_on_resync_pressed);
	ClassDB::bind_method(D_METHOD("_request_task_for_tab_deferred", "tab_id", "user_input"), &UltimateAssistantPanel::_request_task_for_tab_deferred);
}

UltimateAssistantPanel::UltimateAssistantPanel() {
	set_name("Assistant");
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);

	root = memnew(VBoxContainer);
	root->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(root);

	header = memnew(HBoxContainer);
	header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(header);

	title_label = memnew(Label);
	title_label->set_text(TTR("Phoenix Copilot"));
	title_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	header->add_child(title_label);

	new_tab_button = memnew(Button);
	new_tab_button->set_text(TTR("New Chat"));
	new_tab_button->set_tooltip_text(TTR("Open a new assistant tab"));
	new_tab_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_new_tab_pressed));
	header->add_child(new_tab_button);

	settings_button = memnew(Button);
	settings_button->set_text(TTR("Settings"));
	settings_button->set_tooltip_text(TTR("Open assistant settings"));
	settings_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_settings_pressed));
	header->add_child(settings_button);

	root->add_child(memnew(HSeparator));

	tab_container = memnew(TabContainer);
	tab_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	TabBar *tab_bar = tab_container->get_tab_bar();
	if (tab_bar) {
		tab_bar->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_NEVER);
		tab_bar->connect("tab_button_pressed", callable_mp(this, &UltimateAssistantPanel::_on_tab_close_requested));
	}
	root->add_child(tab_container);

	settings_dialog = memnew(UltimateAISettingsDialog);
	settings_dialog->connect(SceneStringName(confirmed), callable_mp(this, &UltimateAssistantPanel::_on_settings_confirmed));
	add_child(settings_dialog);

	backend_adapter = memnew(UltimateAIBackendContractAdapter);
	backend_adapter->apply_runtime_config(settings_dialog->get_runtime_config());
	frontend_runtime_adapter = memnew(UltimateAIFrontendRuntimeAdapter);

	context_dialog = memnew(AcceptDialog);
	context_dialog->set_title(TTR("Add Context"));
	context_dialog->set_ok_button_text(TTR("Add"));
	context_dialog->connect(SceneStringName(confirmed), callable_mp(this, &UltimateAssistantPanel::_on_context_dialog_confirmed));
	add_child(context_dialog);

	VBoxContainer *context_dialog_root = memnew(VBoxContainer);
	context_dialog->add_child(context_dialog_root);

	Label *context_search_label = memnew(Label);
	context_search_label->set_text(TTR("Project files and PixelPen context"));
	context_dialog_root->add_child(context_search_label);

	context_search_input = memnew(LineEdit);
	context_search_input->set_placeholder(TTR("Search files"));
	context_search_input->connect(SceneStringName(text_changed), callable_mp(this, &UltimateAssistantPanel::_on_context_search_changed));
	context_dialog_root->add_child(context_search_input);

	context_file_list = memnew(ItemList);
	context_file_list->set_select_mode(ItemList::SELECT_MULTI);
	context_file_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	context_dialog_root->add_child(context_file_list);

	HBoxContainer *context_pick_buttons = memnew(HBoxContainer);
	context_dialog_root->add_child(context_pick_buttons);

	context_select_add_button = memnew(Button);
	context_select_add_button->set_text(TTR("Add Selected"));
	context_select_add_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_select_add_pressed));
	context_pick_buttons->add_child(context_select_add_button);

	context_select_remove_button = memnew(Button);
	context_select_remove_button->set_text(TTR("Remove Selected"));
	context_select_remove_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_select_remove_pressed));
	context_pick_buttons->add_child(context_select_remove_button);

	Label *context_selected_label = memnew(Label);
	context_selected_label->set_text(TTR("Added context"));
	context_dialog_root->add_child(context_selected_label);

	context_selected_list = memnew(ItemList);
	context_selected_list->set_select_mode(ItemList::SELECT_MULTI);
	context_selected_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	context_dialog_root->add_child(context_selected_list);

	HBoxContainer *context_note_row = memnew(HBoxContainer);
	context_dialog_root->add_child(context_note_row);

	context_note_input = memnew(LineEdit);
	context_note_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	context_note_input->set_placeholder(TTR("Add note"));
	context_note_row->add_child(context_note_input);

	context_note_add_button = memnew(Button);
	context_note_add_button->set_text(TTR("Add Note"));
	context_note_add_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_note_add_pressed));
	context_note_row->add_child(context_note_add_button);

	_ensure_default_models();
	_build_hub_tab();
	_add_chat_tab();
	tab_container->set_current_tab(0);

	s_instances.push_back(this);
	set_process(true);
	if (s_shared_initialized) {
		_apply_shared_state();
	} else {
		_capture_shared_state();
		s_shared_initialized = true;
	}
	_sync_pixelpen_snapshot_to_tabs();
}

void UltimateAssistantPanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE:
		case NOTIFICATION_THEME_CHANGED: {
			theme_ready = true;
			_refresh_tab_close_icons();
		} break;
		case NOTIFICATION_PROCESS: {
			_pump_async_task_requests();
			_pump_async_task_status_polls();
			_pump_async_approvals();
			uint64_t now_msec = OS::get_singleton()->get_ticks_msec();
			for (int i = 0; i < tabs.size(); i++) {
				ChatTab &tab = tabs.write[i];
				_poll_realtime_stream_for_tab(tab.id);

				if (tab.loading_indicator_active && !tab.assistant_stream_open && !tab.command_stream_active && now_msec >= tab.next_loading_tick_msec) {
					_update_loading_chat_indicator_for_tab(tab.id);
					tab.next_loading_tick_msec = now_msec + LOADING_INDICATOR_TICK_MSEC;
				}

				if (tab.command_stream_active && now_msec >= tab.next_command_stream_tick_msec) {
					int chunk_len = MIN(COMMAND_STREAM_CHUNK_CHARS, tab.command_stream_remaining.length());
					if (chunk_len > 0) {
						String chunk = tab.command_stream_remaining.substr(0, chunk_len);
						tab.command_stream_remaining = tab.command_stream_remaining.substr(chunk_len, tab.command_stream_remaining.length() - chunk_len);
						bool finish = tab.command_stream_remaining.is_empty();
						_append_stream_delta_for_tab(tab.id, tab.command_stream_role, chunk, finish);
						if (finish) {
							tab.command_stream_active = false;
							tab.command_stream_role.clear();
							tab.next_command_stream_tick_msec = 0;
							if (!tab.status_poll_active) {
								_set_tab_loading_state(tab.id, false);
								_set_tab_status(tab.id, TTR("Ready"), false);
							}
						} else {
							tab.next_command_stream_tick_msec = now_msec + COMMAND_STREAM_TICK_MSEC;
						}
					} else {
						tab.command_stream_active = false;
						tab.command_stream_role.clear();
						tab.next_command_stream_tick_msec = 0;
					}
				}

				if (!tab.status_poll_active || tab.last_plan_id.is_empty() || tab.has_conflict || now_msec < tab.next_status_poll_msec) {
					continue;
				}

				if (tab.status_poll_request_in_flight) {
					continue;
				}

				tab.status_poll_request_in_flight = true;
				tab.status_poll_generation += 1;
				tab.next_status_poll_msec = now_msec + TASK_STATUS_LIVE_POLL_INTERVAL_MSEC;

				AsyncTaskStatusPollJob *status_job = memnew(AsyncTaskStatusPollJob);
				status_job->tab_id = tab.id;
				status_job->poll_generation = tab.status_poll_generation;
				status_job->plan_id = tab.last_plan_id;
				status_job->one_shot_refresh = false;
				status_job->runtime_config = backend_adapter ? backend_adapter->get_runtime_config() : Dictionary();
				_enqueue_async_task_status_poll(status_job);
			}
		} break;
		case NOTIFICATION_PREDELETE: {
			_drain_async_task_requests();
			_drain_async_task_status_polls();
			_drain_async_approvals();
			for (int i = 0; i < tabs.size(); i++) {
				_disconnect_realtime_stream_for_tab(tabs[i].id);
			}
			for (int i = 0; i < s_instances.size(); i++) {
				if (s_instances[i] == this) {
					s_instances.remove_at(i);
					break;
				}
			}
			if (backend_adapter) {
				memdelete(backend_adapter);
				backend_adapter = nullptr;
			}
			if (frontend_runtime_adapter) {
				memdelete(frontend_runtime_adapter);
				frontend_runtime_adapter = nullptr;
			}
		} break;
		default:
			break;
	}
}

void UltimateAssistantPanel::_ensure_default_models() {
	if (!available_models.is_empty()) {
		return;
	}
	available_models.push_back("gpt-5.2-codex");
	available_models.push_back("claude-sonnet");
	available_models.push_back("claude-haiku");
	available_models.push_back("local-default");
}

void UltimateAssistantPanel::_refresh_model_selectors() {
	for (int i = 0; i < tabs.size(); i++) {
		OptionButton *selector = tabs[i].model_selector;
		if (!selector) {
			continue;
		}
		int selected = selector->get_selected();
		String previous = selected >= 0 ? selector->get_item_text(selected) : "";
		selector->clear();
		for (int j = 0; j < available_models.size(); j++) {
			selector->add_item(available_models[j]);
		}
		if (selector->get_item_count() > 0) {
			int match_index = -1;
			int item_count = selector->get_item_count();
			for (int k = 0; k < item_count; k++) {
				if (selector->get_item_text(k) == previous) {
					match_index = k;
					break;
				}
			}
			selector->select(match_index >= 0 ? match_index : 0);
		}
	}
	_refresh_hub();
}

PackedStringArray UltimateAssistantPanel::_collect_enabled_tool_ids() const {
	PackedStringArray enabled_tools;
	Dictionary runtime_config = backend_adapter ? backend_adapter->get_runtime_config() : Dictionary();

	for (int i = 0; i < TOOL_OPTION_COUNT; i++) {
		const ToolOptionDescriptor &descriptor = TOOL_OPTION_DESCRIPTORS[i];
		bool enabled = true;
		if (runtime_config.has(descriptor.runtime_flag_key)) {
			enabled = bool(runtime_config[descriptor.runtime_flag_key]);
		}
		if (enabled) {
			enabled_tools.push_back(descriptor.id);
		}
	}

	return enabled_tools;
}

PackedStringArray UltimateAssistantPanel::_collect_selected_tool_ids(const ChatTab &p_tab) const {
	PackedStringArray selected_tools;
	if (!p_tab.tool_selector) {
		return selected_tools;
	}

	PackedInt32Array selected_indices = p_tab.tool_selector->get_selected_items();
	for (int i = 0; i < selected_indices.size(); i++) {
		int idx = selected_indices[i];
		if (idx < 0 || idx >= p_tab.tool_selector->get_item_count()) {
			continue;
		}

		String tool_id;
		Variant item_meta = p_tab.tool_selector->get_item_metadata(idx);
		if (item_meta.get_type() == Variant::STRING) {
			tool_id = String(item_meta).strip_edges();
		}
		if (tool_id.is_empty()) {
			tool_id = p_tab.tool_selector->get_item_text(idx).strip_edges();
		}
		if (!tool_id.is_empty() && !selected_tools.has(tool_id)) {
			selected_tools.push_back(tool_id);
		}
	}

	return selected_tools;
}

void UltimateAssistantPanel::_refresh_tool_selector_for_tab(ChatTab &r_tab, const PackedStringArray &p_preferred_selected) {
	if (!r_tab.tool_selector) {
		return;
	}

	PackedStringArray selected_tools = p_preferred_selected;
	if (selected_tools.is_empty()) {
		selected_tools = _collect_selected_tool_ids(r_tab);
	}

	r_tab.tool_selector->clear();
	PackedStringArray enabled_tool_ids = _collect_enabled_tool_ids();
	for (int i = 0; i < enabled_tool_ids.size(); i++) {
		const String tool_id = enabled_tool_ids[i];
		int idx = r_tab.tool_selector->add_item(_tool_label_for_id(tool_id));
		r_tab.tool_selector->set_item_metadata(idx, tool_id);
		if (selected_tools.has(tool_id)) {
			r_tab.tool_selector->select(idx, true);
		}
	}

	bool manual_mode = r_tab.tool_mode_selector && r_tab.tool_mode_selector->get_selected() == 1;
	if (manual_mode && r_tab.tool_selector->get_item_count() > 0 && r_tab.tool_selector->get_selected_items().is_empty()) {
		r_tab.tool_selector->select(0, true);
	}
	r_tab.tool_selector->set_visible(manual_mode);
}

void UltimateAssistantPanel::_refresh_all_tool_selectors() {
	for (int i = 0; i < tabs.size(); i++) {
		ChatTab &tab = tabs.write[i];
		_refresh_tool_selector_for_tab(tab);
	}
}

void UltimateAssistantPanel::_build_hub_tab() {
	hub_root = memnew(VBoxContainer);
	hub_root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	HBoxContainer *hub_header = memnew(HBoxContainer);
	hub_header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->add_child(hub_header);

	Label *hub_title = memnew(Label);
	hub_title->set_text(TTR("Agent Hub"));
	hub_title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hub_header->add_child(hub_title);

	hub_root->add_child(memnew(HSeparator));

	Label *hub_list_label = memnew(Label);
	hub_list_label->set_text(TTR("Active agents"));
	hub_root->add_child(hub_list_label);

	hub_agent_list = memnew(ItemList);
	hub_agent_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->add_child(hub_agent_list);

	hub_root->add_child(memnew(HSeparator));

	Label *previous_label = memnew(Label);
	previous_label->set_text(TTR("Previous sessions"));
	hub_root->add_child(previous_label);

	hub_previous_list = memnew(ItemList);
	hub_previous_list->set_select_mode(ItemList::SELECT_SINGLE);
	hub_previous_list->set_allow_reselect(true);
	hub_previous_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hub_previous_list->connect("item_activated", callable_mp(this, &UltimateAssistantPanel::_on_previous_session_activated));
	hub_root->add_child(hub_previous_list);

	hub_reopen_button = memnew(Button);
	hub_reopen_button->set_text(TTR("Reopen Session"));
	hub_reopen_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_previous_reopen_pressed));
	hub_root->add_child(hub_reopen_button);

	hub_root->add_child(memnew(HSeparator));

	Label *pending_label = memnew(Label);
	pending_label->set_text(TTR("Approvals & pending actions"));
	hub_root->add_child(pending_label);

	hub_pending_list = memnew(ItemList);
	hub_pending_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->add_child(hub_pending_list);

	hub_root->add_child(memnew(HSeparator));

	Label *questions_label = memnew(Label);
	questions_label->set_text(TTR("Agent questions"));
	hub_root->add_child(questions_label);

	hub_questions_list = memnew(ItemList);
	hub_questions_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->add_child(hub_questions_list);

	tab_container->add_child(hub_root);
	tab_container->set_tab_title(tab_container->get_tab_count() - 1, TTR("Hub"));
	_refresh_tab_close_icons();
}

void UltimateAssistantPanel::_add_chat_tab(int p_forced_id) {
	ChatTab tab;
	VBoxContainer *tab_root = memnew(VBoxContainer);
	tab_root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab_root->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	OptionButton *mode_selector = memnew(OptionButton);
	mode_selector->add_item(TTR("Ask"));
	mode_selector->add_item(TTR("Plan"));
	mode_selector->add_item(TTR("Agent"));
	mode_selector->set_tooltip_text(TTR("Request mode: Ask for guidance, Plan for a structured plan, Agent for execution-ready output."));

	OptionButton *model_selector = memnew(OptionButton);
	for (int i = 0; i < available_models.size(); i++) {
		model_selector->add_item(available_models[i]);
	}
	if (model_selector->get_item_count() > 0) {
		model_selector->select(0);
	}
	model_selector->set_tooltip_text(TTR("Model (per chat)"));

	OptionButton *agent_mode_selector = memnew(OptionButton);
	agent_mode_selector->add_item(TTR("Local Agent"));
	agent_mode_selector->add_item(TTR("Background Agent"));
	agent_mode_selector->set_tooltip_text(TTR("Execution target preference for this chat tab."));

	OptionButton *tool_mode_selector = memnew(OptionButton);
	tool_mode_selector->add_item(TTR("Tools: Auto"));
	tool_mode_selector->add_item(TTR("Tools: Manual"));
	tool_mode_selector->set_tooltip_text(TTR("Auto lets Phoenix choose tools. Manual uses only selected tools."));

	LineEdit *session_name_input = memnew(LineEdit);
	session_name_input->set_placeholder(TTR("Session name"));
	session_name_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);

	tab_root->add_child(memnew(HSeparator));

	VSplitContainer *chat_split = memnew(VSplitContainer);
	chat_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	chat_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_split->set_split_offset(350);
	tab_root->add_child(chat_split);

	ScrollContainer *scroll_container = memnew(ScrollContainer);
	scroll_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_split->add_child(scroll_container);

	RichTextLabel *chat_display = memnew(RichTextLabel);
	chat_display->set_use_bbcode(true);
	chat_display->set_selection_enabled(true);
	chat_display->set_context_menu_enabled(true);
	chat_display->set_shortcut_keys_enabled(true);
	chat_display->set_scroll_follow(true);
	chat_display->add_theme_constant_override("line_separation", 6);
	chat_display->add_theme_constant_override("paragraph_separation", 8);
	chat_display->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	chat_display->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_display->set_text(TTR("[i]New chat ready.[/i]\n"));
	scroll_container->add_child(chat_display);

	VBoxContainer *input_block = memnew(VBoxContainer);
	input_block->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_block->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_split->add_child(input_block);

	HBoxContainer *controls_row = memnew(HBoxContainer);
	controls_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_block->add_child(controls_row);
	controls_row->add_child(mode_selector);
	controls_row->add_child(model_selector);
	controls_row->add_child(agent_mode_selector);
	controls_row->add_child(tool_mode_selector);
	controls_row->add_child(session_name_input);

	ItemList *tool_selector = memnew(ItemList);
	tool_selector->set_select_mode(ItemList::SELECT_MULTI);
	tool_selector->set_v_size_flags(Control::SIZE_FILL);
	tool_selector->set_custom_minimum_size(Size2(0, 70));
	tool_selector->set_visible(false);
	input_block->add_child(tool_selector);

	VBoxContainer *context_section = memnew(VBoxContainer);
	context_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_block->add_child(context_section);

	HBoxContainer *context_row = memnew(HBoxContainer);
	context_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	context_section->add_child(context_row);

	Button *context_toggle = memnew(Button);
	context_toggle->set_text(TTR("Context"));
	context_toggle->set_toggle_mode(true);
	context_toggle->set_pressed(true);
	context_row->add_child(context_toggle);

	Button *context_add_button = memnew(Button);
	context_add_button->set_text(TTR("Add"));
	context_row->add_child(context_add_button);

	Button *context_remove_button = memnew(Button);
	context_remove_button->set_text(TTR("Remove"));
	context_row->add_child(context_remove_button);

	ItemList *context_list = memnew(ItemList);
	context_list->set_select_mode(ItemList::SELECT_MULTI);
	context_list->set_v_size_flags(Control::SIZE_FILL);
	context_list->set_custom_minimum_size(Size2(0, 80));
	context_section->add_child(context_list);

	TextEdit *input_text = memnew(TextEdit);
	input_text->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_text->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	input_text->set_placeholder(TTR("Type a message..."));
	input_text->set_custom_minimum_size(Size2(0, 90));
	input_block->add_child(input_text);

	HBoxContainer *input_actions = memnew(HBoxContainer);
	input_actions->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_block->add_child(input_actions);

	Control *actions_spacer = memnew(Control);
	actions_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_actions->add_child(actions_spacer);

	Button *send_button = memnew(Button);
	send_button->set_text(TTR("Send"));
	input_actions->add_child(send_button);

	Button *copy_button = memnew(Button);
	copy_button->set_text(TTR("Copy"));
	copy_button->set_tooltip_text(TTR("Copy selected text or full chat transcript"));
	input_actions->add_child(copy_button);

	Button *steer_button = memnew(Button);
	steer_button->set_text(TTR("Steer"));
	steer_button->set_visible(false);
	input_actions->add_child(steer_button);

	Button *resync_button = memnew(Button);
	resync_button->set_text(TTR("Resync"));
	resync_button->set_visible(false);
	input_actions->add_child(resync_button);

	Label *status_label = memnew(Label);
	status_label->set_text(TTR("Ready"));
	input_block->add_child(status_label);

	VBoxContainer *approval_section = memnew(VBoxContainer);
	approval_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	approval_section->set_visible(false);
	input_block->add_child(approval_section);

	Label *approval_label = memnew(Label);
	approval_label->set_text(TTR("Pending approvals"));
	approval_section->add_child(approval_label);

	ItemList *approval_list = memnew(ItemList);
	approval_list->set_v_size_flags(Control::SIZE_FILL);
	approval_list->set_custom_minimum_size(Size2(0, 80));
	approval_section->add_child(approval_list);

	HBoxContainer *approval_actions = memnew(HBoxContainer);
	approval_section->add_child(approval_actions);

	Button *approve_button = memnew(Button);
	approve_button->set_text(TTR("Approve"));
	approval_actions->add_child(approve_button);

	Button *reject_button = memnew(Button);
	reject_button->set_text(TTR("Reject"));
	approval_actions->add_child(reject_button);

	int tab_id = p_forced_id >= 0 ? p_forced_id : (tab_counter + 1);
	if (tab_id > tab_counter) {
		tab_counter = tab_id;
	}
	send_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_send_pressed).bind(tab_id));
	copy_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_copy_pressed).bind(tab_id));
	steer_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_steer_pressed).bind(tab_id));
	resync_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_resync_pressed).bind(tab_id));
	approve_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_approve_pressed).bind(tab_id));
	reject_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_reject_pressed).bind(tab_id));
	mode_selector->connect(SceneStringName(item_selected), callable_mp(this, &UltimateAssistantPanel::_on_tab_setting_changed).bind(tab_id));
	model_selector->connect(SceneStringName(item_selected), callable_mp(this, &UltimateAssistantPanel::_on_tab_setting_changed).bind(tab_id));
	agent_mode_selector->connect(SceneStringName(item_selected), callable_mp(this, &UltimateAssistantPanel::_on_tab_setting_changed).bind(tab_id));
	tool_mode_selector->connect(SceneStringName(item_selected), callable_mp(this, &UltimateAssistantPanel::_on_tab_setting_changed).bind(tab_id));
	tool_selector->connect("multi_selected", callable_mp(this, &UltimateAssistantPanel::_on_tool_selection_changed).bind(tab_id));
	context_add_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_add_pressed).bind(tab_id));
	context_remove_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_remove_pressed).bind(tab_id));
	context_toggle->connect(SceneStringName(toggled), callable_mp(this, &UltimateAssistantPanel::_on_context_toggle_toggled).bind(tab_id));
	session_name_input->connect(SceneStringName(text_changed), callable_mp(this, &UltimateAssistantPanel::_on_session_name_changed).bind(tab_id));

	String tab_title = vformat("Agent %d", tab_id);
	tab_container->add_child(tab_root);
	tab_container->set_tab_title(tab_container->get_tab_count() - 1, tab_title);

	tab.id = tab_id;
	tab.root = tab_root;
	tab.mode_selector = mode_selector;
	tab.model_selector = model_selector;
	tab.agent_mode_selector = agent_mode_selector;
	tab.tool_mode_selector = tool_mode_selector;
	tab.tool_selector = tool_selector;
	tab.session_name_input = session_name_input;
	tab.context_list = context_list;
	tab.context_section = context_section;
	tab.context_toggle_button = context_toggle;
	tab.context_add_button = context_add_button;
	tab.context_remove_button = context_remove_button;
	tab.chat_display = chat_display;
	tab.input_text = input_text;
	tab.status_label = status_label;
	tab.send_button = send_button;
	tab.copy_button = copy_button;
	tab.steer_button = steer_button;
	tab.resync_button = resync_button;
	tab.approval_section = approval_section;
	tab.approval_label = approval_label;
	tab.approval_list = approval_list;
	tab.approve_button = approve_button;
	tab.reject_button = reject_button;
	tab.transcript = chat_display->get_text();

	tabs.push_back(tab);
	_refresh_tool_selector_for_tab(tabs.write[tabs.size() - 1]);
	tab_container->set_current_tab(tab_container->get_tab_count() - 1);
	_refresh_tab_close_icons();
	_refresh_hub();
}

UltimateAssistantPanel::SharedChatTabState UltimateAssistantPanel::_build_shared_state_for_tab(const ChatTab &p_tab) const {
	SharedChatTabState state;
	state.id = p_tab.id;
	state.display_name = p_tab.display_name;
	state.transcript = p_tab.transcript;
	state.mode_selected = p_tab.mode_selector ? p_tab.mode_selector->get_selected() : 0;
	state.model_selected = p_tab.model_selector ? p_tab.model_selector->get_selected() : 0;
	state.agent_mode_selected = p_tab.agent_mode_selector ? p_tab.agent_mode_selector->get_selected() : 0;
	state.tool_mode_selected = p_tab.tool_mode_selector ? p_tab.tool_mode_selector->get_selected() : 0;
	state.session_name_text = p_tab.session_name_input ? p_tab.session_name_input->get_text() : p_tab.display_name;
	state.selected_tool_ids = _collect_selected_tool_ids(p_tab);
	state.context_collapsed = p_tab.context_collapsed;
	state.is_active = p_tab.is_active;
	if (p_tab.context_list) {
		int count = p_tab.context_list->get_item_count();
		for (int i = 0; i < count; i++) {
			state.context_items.push_back(p_tab.context_list->get_item_text(i));
			state.context_metadata.push_back(p_tab.context_list->get_item_metadata(i));
		}
	}
	return state;
}

void UltimateAssistantPanel::_capture_shared_state() {
	s_shared_tabs.clear();
	for (int i = 0; i < tabs.size(); i++) {
		s_shared_tabs.push_back(_build_shared_state_for_tab(tabs[i]));
	}
	s_shared_archived_sessions = archived_sessions;
	s_shared_models = available_models;
	s_shared_tab_counter = tab_counter;
	s_shared_current_tab = tab_container ? tab_container->get_current_tab() : 0;
}

void UltimateAssistantPanel::_clear_tabs_ui() {
	for (int i = 0; i < tabs.size(); i++) {
		_disconnect_realtime_stream_for_tab(tabs[i].id);
	}
	tabs.clear();
	hub_root = nullptr;
	hub_agent_list = nullptr;
	hub_previous_list = nullptr;
	hub_reopen_button = nullptr;
	hub_pending_list = nullptr;
	hub_questions_list = nullptr;
	if (!tab_container) {
		return;
	}
	for (int i = tab_container->get_tab_count() - 1; i >= 0; i--) {
		Control *tab_control = tab_container->get_tab_control(i);
		if (!tab_control) {
			continue;
		}
		tab_container->remove_child(tab_control);
		tab_control->queue_free();
	}
}

void UltimateAssistantPanel::_add_chat_tab_from_state(const SharedChatTabState &p_state) {
	_add_chat_tab(p_state.id);
	int tab_index = _find_tab_index_by_id(p_state.id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];

	tab.display_name = p_state.display_name;
	tab.transcript = p_state.transcript;
	tab.context_collapsed = p_state.context_collapsed;
	tab.is_active = p_state.is_active;

	if (tab.mode_selector && tab.mode_selector->get_item_count() > 0) {
		int idx = p_state.mode_selected;
		if (idx < 0) {
			idx = 0;
		} else if (idx >= tab.mode_selector->get_item_count()) {
			idx = tab.mode_selector->get_item_count() - 1;
		}
		tab.mode_selector->select(idx);
	}
	if (tab.model_selector && tab.model_selector->get_item_count() > 0) {
		int idx = p_state.model_selected;
		if (idx < 0) {
			idx = 0;
		} else if (idx >= tab.model_selector->get_item_count()) {
			idx = tab.model_selector->get_item_count() - 1;
		}
		tab.model_selector->select(idx);
	}
	if (tab.agent_mode_selector && tab.agent_mode_selector->get_item_count() > 0) {
		int idx = p_state.agent_mode_selected;
		if (idx < 0) {
			idx = 0;
		} else if (idx >= tab.agent_mode_selector->get_item_count()) {
			idx = tab.agent_mode_selector->get_item_count() - 1;
		}
		tab.agent_mode_selector->select(idx);
	}
	if (tab.tool_mode_selector && tab.tool_mode_selector->get_item_count() > 0) {
		int idx = p_state.tool_mode_selected;
		if (idx < 0) {
			idx = 0;
		} else if (idx >= tab.tool_mode_selector->get_item_count()) {
			idx = tab.tool_mode_selector->get_item_count() - 1;
		}
		tab.tool_mode_selector->select(idx);
	}
	_refresh_tool_selector_for_tab(tab, p_state.selected_tool_ids);
	if (tab.session_name_input) {
		tab.session_name_input->set_text(p_state.session_name_text);
	}
	if (tab.context_toggle_button) {
		tab.context_toggle_button->set_pressed(!p_state.context_collapsed);
	}
	if (tab.context_list) {
		tab.context_list->set_visible(!p_state.context_collapsed);
		tab.context_list->clear();
		for (int i = 0; i < p_state.context_items.size(); i++) {
			int item_idx = tab.context_list->add_item(p_state.context_items[i]);
			Variant meta = p_state.context_metadata.size() > i ? p_state.context_metadata[i] : Variant();
			if (meta.get_type() != Variant::NIL) {
				tab.context_list->set_item_metadata(item_idx, meta);
			}
		}
	}
	if (tab.chat_display) {
		tab.chat_display->set_text(p_state.transcript.is_empty() ? TTR("[i]New chat ready.[/i]\n") : p_state.transcript);
		tab.transcript = tab.chat_display->get_text();
	}
	if (tab.send_button) {
		tab.send_button->set_text(p_state.is_active ? TTR("Interrupt") : TTR("Send"));
	}
	if (tab.steer_button) {
		tab.steer_button->set_visible(p_state.is_active);
	}
	if (tab.status_label) {
		tab.status_label->set_text(p_state.is_active ? TTR("Busy") : TTR("Ready"));
	}
	if (tab.resync_button) {
		tab.resync_button->set_visible(false);
	}
	if (tab.approval_section) {
		tab.approval_section->set_visible(false);
	}

	int container_index = _find_tab_container_index(tab.root);
	if (container_index >= 0) {
		tab_container->set_tab_title(container_index, _get_tab_label(tab));
	}
}

void UltimateAssistantPanel::_apply_shared_state() {
	if (!s_shared_initialized) {
		return;
	}

	applying_shared_state = true;
	_clear_tabs_ui();

	available_models = s_shared_models;
	if (available_models.is_empty()) {
		_ensure_default_models();
	}
	archived_sessions = s_shared_archived_sessions;
	tab_counter = 0;

	_build_hub_tab();
	if (s_shared_tabs.is_empty()) {
		_add_chat_tab();
	} else {
		for (int i = 0; i < s_shared_tabs.size(); i++) {
			_add_chat_tab_from_state(s_shared_tabs[i]);
		}
	}

	if (s_shared_tab_counter > tab_counter) {
		tab_counter = s_shared_tab_counter;
	}

	int tab_count = tab_container ? tab_container->get_tab_count() : 0;
	if (tab_container && tab_count > 0) {
		int target_tab = s_shared_current_tab;
		if (target_tab < 0) {
			target_tab = 0;
		} else if (target_tab >= tab_count) {
			target_tab = tab_count - 1;
		}
		tab_container->set_current_tab(target_tab);
	}

	_refresh_hub();
	_refresh_tab_close_icons();
	applying_shared_state = false;
	_sync_pixelpen_snapshot_to_tabs();
}

void UltimateAssistantPanel::_broadcast_shared_state() {
	if (applying_shared_state) {
		return;
	}
	_capture_shared_state();
	s_shared_initialized = true;
	for (int i = 0; i < s_instances.size(); i++) {
		UltimateAssistantPanel *panel = s_instances[i];
		if (!panel || panel == this) {
			continue;
		}
		panel->_apply_shared_state();
	}
}

void UltimateAssistantPanel::_upsert_pixelpen_snapshot(ChatTab &r_tab) {
	if (!r_tab.context_list) {
		return;
	}

	int existing = -1;
	int count = r_tab.context_list->get_item_count();
	for (int i = 0; i < count; i++) {
		Variant meta = r_tab.context_list->get_item_metadata(i);
		if (meta.get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary meta_dict = meta;
		if (meta_dict.get("kind", String()) == PIXELPEN_SNAPSHOT_KIND) {
			existing = i;
			break;
		}
	}

	if (s_shared_pixelpen_snapshot.is_empty()) {
		if (existing >= 0) {
			r_tab.context_list->remove_item(existing);
		}
		return;
	}

	String label = "PixelPen Snapshot";
	if (s_shared_pixelpen_snapshot.has("project_name")) {
		String project_name = s_shared_pixelpen_snapshot["project_name"];
		if (!project_name.is_empty()) {
			label += ": " + project_name;
		}
	} else if (s_shared_pixelpen_snapshot.has("window_title")) {
		String window_title = s_shared_pixelpen_snapshot["window_title"];
		if (!window_title.is_empty()) {
			label += ": " + window_title;
		}
	}

	Dictionary context_metadata;
	context_metadata["kind"] = PIXELPEN_SNAPSHOT_KIND;
	context_metadata["source"] = "pixelpen";
	context_metadata["snapshot"] = s_shared_pixelpen_snapshot;
	context_metadata["layers"] = s_shared_pixelpen_layers;

	if (existing >= 0) {
		r_tab.context_list->set_item_text(existing, label);
		r_tab.context_list->set_item_metadata(existing, context_metadata);
	} else {
		int idx = r_tab.context_list->add_item(label);
		r_tab.context_list->set_item_metadata(idx, context_metadata);
	}
}

void UltimateAssistantPanel::_sync_pixelpen_snapshot_to_tabs() {
	for (int i = 0; i < tabs.size(); i++) {
		ChatTab &tab = tabs.write[i];
		_upsert_pixelpen_snapshot(tab);
	}
	if (context_dialog && context_dialog->is_visible()) {
		String filter = context_search_input ? context_search_input->get_text() : String();
		_populate_context_file_list(filter);
		_sync_context_selected_list(context_dialog_tab_id);
	}
}

void UltimateAssistantPanel::_populate_pixelpen_context_items(const String &p_filter) {
	if (!context_file_list) {
		return;
	}
	String filter = p_filter.strip_edges();

	if (!s_shared_pixelpen_snapshot.is_empty()) {
		String label = "[PixelPen] Snapshot";
		String project_name = s_shared_pixelpen_snapshot.get("project_name", String());
		if (!project_name.is_empty()) {
			label += ": " + project_name;
		}
		if (filter.is_empty() || label.findn(filter) != -1) {
			Dictionary snapshot_meta;
			snapshot_meta["kind"] = PIXELPEN_SNAPSHOT_KIND;
			snapshot_meta["source"] = "pixelpen";
			snapshot_meta["snapshot"] = s_shared_pixelpen_snapshot;
			snapshot_meta["layers"] = s_shared_pixelpen_layers;
			int snapshot_idx = context_file_list->add_item(label);
			context_file_list->set_item_metadata(snapshot_idx, snapshot_meta);
		}
	}

	for (int i = 0; i < s_shared_pixelpen_layers.size(); i++) {
		Variant entry = s_shared_pixelpen_layers[i];
		if (entry.get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary layer = entry;
		String layer_label = layer.get("label", String("Layer"));
		bool active = layer.get("active", false);
		bool layer_visible = layer.get("visible", true);
		String label = vformat("[PixelPen] Layer: %s%s%s", layer_label, active ? " (Active)" : "", layer_visible ? "" : " (Hidden)");
		if (!filter.is_empty() && label.findn(filter) == -1) {
			continue;
		}
		Dictionary layer_meta;
		layer_meta["kind"] = PIXELPEN_LAYER_KIND;
		layer_meta["source"] = "pixelpen";
		layer_meta["layer"] = layer;
		layer_meta["snapshot"] = s_shared_pixelpen_snapshot;
		int idx = context_file_list->add_item(label);
		context_file_list->set_item_metadata(idx, layer_meta);
	}
}

void UltimateAssistantPanel::broadcast_pixelpen_context(const Dictionary &p_snapshot, const Array &p_layers) {
	s_shared_pixelpen_snapshot = p_snapshot;
	s_shared_pixelpen_layers = p_layers;
	for (int i = 0; i < s_instances.size(); i++) {
		UltimateAssistantPanel *panel = s_instances[i];
		if (!panel) {
			continue;
		}
		panel->_sync_pixelpen_snapshot_to_tabs();
	}
	if (!s_instances.is_empty()) {
		s_instances[0]->_broadcast_shared_state();
	}
}

void UltimateAssistantPanel::_append_message(int p_tab_id, const String &p_role, const String &p_content) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	String safe_role = p_role.strip_edges();
	String safe_content = p_content.strip_edges();
	if (safe_content.is_empty()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	RichTextLabel *display = tab.chat_display;
	if (!display) {
		return;
	}
	_clear_loading_chat_indicator_for_tab(p_tab_id);
	if (tab.command_stream_active) {
		String incoming_role_key = safe_role.to_lower();
		if (incoming_role_key.is_empty()) {
			incoming_role_key = "assistant";
		}

		String stream_role_key = tab.command_stream_role.strip_edges().to_lower();
		if (stream_role_key.is_empty()) {
			stream_role_key = "assistant";
		}

		if (incoming_role_key != stream_role_key) {
			if (!tab.command_stream_remaining.is_empty()) {
				String remaining = tab.command_stream_remaining;
				tab.command_stream_remaining.clear();
				_append_stream_delta_for_tab(p_tab_id, tab.command_stream_role, remaining, true);
			} else if (tab.assistant_stream_open) {
				_append_stream_delta_for_tab(p_tab_id, String(), String(), true);
			}

			tab.command_stream_active = false;
			tab.command_stream_role.clear();
			tab.next_command_stream_tick_msec = 0;
		}
	}
	if (tab.assistant_stream_open) {
		_append_stream_delta_for_tab(p_tab_id, String(), String(), true);
		tab.assistant_stream_from_realtime = false;
	}

	String rendered_content = _markdown_to_bbcode(safe_content);
	String safe_role_bbcode = _escape_bbcode_text(safe_role);

	const bool is_user = safe_role == TTR("User");
	const bool is_system = safe_role == TTR("System");

	Color base_color = display->get_theme_color(SNAME("default_color"), SNAME("RichTextLabel"));
	if (base_color.a <= 0.0) {
		base_color = Color(1, 1, 1, 1);
	}

	Color role_color = base_color.lightened(0.35);
	Color bubble_color = base_color.darkened(0.82);
	if (is_user) {
		role_color = base_color.lightened(0.5);
		bubble_color = base_color.darkened(0.68);
	} else if (is_system) {
		role_color = base_color.lightened(0.2);
		bubble_color = base_color.darkened(0.9);
	}

	String role_color_hex = "#" + role_color.to_html(false);
	String bubble_color_hex = "#" + bubble_color.to_html(false);
	String text_color_hex = "#" + base_color.to_html(false);

	String line;
	if (is_user) {
		line += "[right][color=" + role_color_hex + "][b]" + safe_role_bbcode + "[/b][/color][/right]\n";
		line += "[right][bgcolor=" + bubble_color_hex + "][color=" + text_color_hex + "]\n  " + rendered_content + "\n[/color][/bgcolor][/right]\n\n";
	} else {
		line += "[color=" + role_color_hex + "][b]" + safe_role_bbcode + "[/b][/color]\n";
		line += "[bgcolor=" + bubble_color_hex + "][color=" + text_color_hex + "]\n  " + rendered_content + "\n[/color][/bgcolor]\n\n";
	}

	display->append_text(line);
	tab.transcript += line;
	display->scroll_to_line(display->get_line_count());

	Color display_modulate = display->get_modulate();
	display_modulate.a = 0.92;
	display->set_modulate(display_modulate);
	Ref<Tween> tween = display->create_tween();
	if (tween.is_valid()) {
		tween->set_trans(Tween::TRANS_SINE);
		tween->set_ease(Tween::EASE_OUT);
		tween->tween_property(display, NodePath("modulate:a"), 1.0, 0.14);
	}

	_broadcast_shared_state();
}

void UltimateAssistantPanel::_append_stream_delta_for_tab(int p_tab_id, const String &p_role, const String &p_delta, bool p_finish) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	RichTextLabel *display = tab.chat_display;
	if (!display) {
		return;
	}
	_clear_loading_chat_indicator_for_tab(p_tab_id);
	if (!p_delta.is_empty() || p_finish) {
		_append_thinking_delta_for_tab(p_tab_id, String(), true);
	}

	if (!tab.assistant_stream_open && !p_finish) {
		String safe_role = p_role.strip_edges();
		if (safe_role.is_empty()) {
			safe_role = TTR("Assistant");
		}

		Color base_color = display->get_theme_color(SNAME("default_color"), SNAME("RichTextLabel"));
		if (base_color.a <= 0.0) {
			base_color = Color(1, 1, 1, 1);
		}
		Color role_color = base_color.lightened(0.35);
		Color bubble_color = base_color.darkened(0.82);

		String line;
		line += "[color=#" + role_color.to_html(false) + "][b]" + _escape_bbcode_text(safe_role) + "[/b][/color]\n";
		line += "[bgcolor=#" + bubble_color.to_html(false) + "][color=#" + base_color.to_html(false) + "]\n  ";
		tab.assistant_stream_prefix_len = tab.transcript.length();
		display->append_text(line);
		tab.transcript += line;
		tab.assistant_stream_open = true;
		tab.assistant_stream_text.clear();
	}

	if (tab.assistant_stream_open && !p_delta.is_empty()) {
		String delta_to_append = p_delta;
		if (!tab.assistant_stream_text.is_empty()) {
			int overlap = _suffix_prefix_overlap_length(tab.assistant_stream_text, delta_to_append);
			if (overlap > 0) {
				delta_to_append = delta_to_append.substr(overlap, delta_to_append.length() - overlap);
			}
		}

		if (delta_to_append.is_empty() && !p_finish) {
			display->scroll_to_line(display->get_line_count());
			return;
		}

		if (!delta_to_append.is_empty()) {
			String safe_chunk = _escape_bbcode_text(delta_to_append);
			display->append_text(safe_chunk);
			tab.transcript += safe_chunk;
			tab.assistant_stream_text += delta_to_append;
		}
	}

	if (tab.assistant_stream_open && p_finish) {
		String closing = "\n[/color][/bgcolor]\n\n";
		display->append_text(closing);
		tab.transcript += closing;
		tab.assistant_stream_open = false;
		tab.assistant_stream_from_realtime = false;
		_broadcast_shared_state();
	}

	display->scroll_to_line(display->get_line_count());
}

void UltimateAssistantPanel::_append_thinking_delta_for_tab(int p_tab_id, const String &p_delta, bool p_finish) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	RichTextLabel *display = tab.chat_display;
	if (!display) {
		return;
	}

	if (!tab.thinking_stream_open && !p_finish && !p_delta.is_empty()) {
		Color base_color = display->get_theme_color(SNAME("default_color"), SNAME("RichTextLabel"));
		if (base_color.a <= 0.0) {
			base_color = Color(1, 1, 1, 1);
		}
		Color thinking_color = base_color.lightened(0.2);

		String header_text = "[color=#" + thinking_color.to_html(false) + "][i]" + _escape_bbcode_text(TTR("Thinking")) + ": [/i][/color]";
		tab.thinking_stream_prefix_len = tab.transcript.length();
		display->append_text(header_text);
		tab.transcript += header_text;
		tab.thinking_stream_open = true;
		tab.thinking_stream_text.clear();
	}

	if (tab.thinking_stream_open && !p_delta.is_empty()) {
		String safe_chunk = _escape_bbcode_text(p_delta);
		display->append_text(safe_chunk);
		tab.transcript += safe_chunk;
		tab.thinking_stream_text += p_delta;
	}

	if (tab.thinking_stream_open && p_finish) {
		String closing = "\n\n";
		display->append_text(closing);
		tab.transcript += closing;
		tab.thinking_stream_open = false;
		tab.thinking_stream_text.clear();
		tab.thinking_stream_prefix_len = 0;
		_broadcast_shared_state();
	}

	display->scroll_to_line(display->get_line_count());
}

void UltimateAssistantPanel::_on_new_tab_pressed() {
	_add_chat_tab();
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_on_tab_close_requested(int p_tab_index) {
	if (p_tab_index == 0) {
		return;
	}
	if (p_tab_index < 0 || p_tab_index >= tab_container->get_tab_count()) {
		return;
	}
	Control *tab_control = tab_container->get_tab_control(p_tab_index);
	int tab_index = _find_tab_index_by_root(tab_control);
	if (tab_index >= 0 && tab_index < tabs.size()) {
		_disconnect_realtime_stream_for_tab(tabs[tab_index].id);
		_archive_tab(tabs[tab_index]);
	}
	if (tab_control) {
		tab_container->remove_child(tab_control);
		tab_control->queue_free();
	}
	if (tab_index >= 0 && tab_index < tabs.size()) {
		tabs.remove_at(tab_index);
	}

	if (tab_container->get_tab_count() == 0) {
		_build_hub_tab();
		_add_chat_tab();
	}
	_refresh_tab_close_icons();
	_refresh_hub();
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_refresh_tab_close_icons() {
	if (!tab_container || !theme_ready) {
		return;
	}
	Ref<Texture2D> close_icon = get_theme_icon(SNAME("Close"), SNAME("EditorIcons"));
	if (!close_icon.is_valid() || close_icon->get_width() <= 0 || close_icon->get_height() <= 0) {
		return;
	}
	int tab_count = tab_container->get_tab_count();
	for (int i = 0; i < tab_count; i++) {
		if (i == 0) {
			tab_container->set_tab_button_icon(i, Ref<Texture2D>());
			continue;
		}
		tab_container->set_tab_button_icon(i, close_icon);
	}
}

int UltimateAssistantPanel::_find_tab_index_by_id(int p_tab_id) const {
	for (int i = 0; i < tabs.size(); i++) {
		if (tabs[i].id == p_tab_id) {
			return i;
		}
	}
	return -1;
}

int UltimateAssistantPanel::_find_tab_index_by_root(Control *p_root) const {
	if (!p_root) {
		return -1;
	}
	for (int i = 0; i < tabs.size(); i++) {
		if (tabs[i].root == p_root) {
			return i;
		}
	}
	return -1;
}

int UltimateAssistantPanel::_find_tab_container_index(Control *p_root) const {
	if (!tab_container || !p_root) {
		return -1;
	}
	int tab_count = tab_container->get_tab_count();
	for (int i = 0; i < tab_count; i++) {
		if (tab_container->get_tab_control(i) == p_root) {
			return i;
		}
	}
	return -1;
}

void UltimateAssistantPanel::_on_send_pressed(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	if (tab.is_active) {
		_on_interrupt_pressed(p_tab_id);
		return;
	}
	TextEdit *input = tab.input_text;
	if (!input) {
		return;
	}
	String request_text = input->get_text().strip_edges();
	if (request_text.is_empty()) {
		return;
	}
	_on_input_submitted(request_text, p_tab_id);
	_set_tab_loading_state(p_tab_id, true, TTR("Assistant is thinking"));
	call_deferred(SNAME("_request_task_for_tab_deferred"), p_tab_id, request_text);
}

void UltimateAssistantPanel::_request_task_for_tab_deferred(int p_tab_id, const String &p_user_input) {
	_request_task_for_tab(p_tab_id, p_user_input);
}

void UltimateAssistantPanel::_on_copy_pressed(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	RichTextLabel *display = tabs[tab_index].chat_display;
	if (!display) {
		return;
	}

	String selected_text = display->get_selected_text();
	String copy_text = selected_text.strip_edges().is_empty() ? display->get_parsed_text() : selected_text;
	if (copy_text.strip_edges().is_empty()) {
		return;
	}

	DisplayServer::get_singleton()->clipboard_set(copy_text);
	_set_tab_status(p_tab_id, TTR("Copied chat text."), false);
}

void UltimateAssistantPanel::_on_input_submitted(const String &p_text, int p_tab_id) {
	String trimmed = p_text.strip_edges();
	if (trimmed.is_empty()) {
		return;
	}
	_append_message(p_tab_id, TTR("User"), trimmed);
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index >= 0 && tab_index < tabs.size()) {
		if (tabs[tab_index].input_text) {
			tabs[tab_index].input_text->clear();
		}
	}
}

void UltimateAssistantPanel::_on_interrupt_pressed(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	_append_message(p_tab_id, TTR("User"), TTR("[interrupt]"));
	_set_tab_loading_state(p_tab_id, true, TTR("Assistant is thinking"));
	call_deferred(SNAME("_request_task_for_tab_deferred"), p_tab_id, String("[interrupt]"));
}

void UltimateAssistantPanel::_on_steer_pressed(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	String steer_prompt = TTR("Please steer the current plan toward the latest user intent.");
	if (tabs[tab_index].input_text) {
		String candidate = tabs[tab_index].input_text->get_text().strip_edges();
		if (!candidate.is_empty()) {
			steer_prompt = candidate;
			tabs[tab_index].input_text->clear();
		}
	}
	_append_message(p_tab_id, TTR("User"), "[steer] " + steer_prompt);
	_set_tab_loading_state(p_tab_id, true, TTR("Assistant is thinking"));
	call_deferred(SNAME("_request_task_for_tab_deferred"), p_tab_id, String("[steer] ") + steer_prompt);
}

void UltimateAssistantPanel::_set_tab_busy(int p_tab_id, bool p_busy, const String &p_status) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	tab.is_active = p_busy;
	if (!p_busy) {
		tab.status_poll_active = false;
		tab.next_status_poll_msec = 0;
		tab.last_reported_task_status.clear();
		tab.task_request_in_flight = false;
		tab.status_poll_request_in_flight = false;
		tab.approval_request_in_flight = false;
	}

	if (tab.send_button) {
		tab.send_button->set_text(p_busy ? TTR("Interrupt") : TTR("Send"));
	}
	if (tab.steer_button) {
		tab.steer_button->set_visible(p_busy);
	}
	if (!p_status.is_empty()) {
		_set_tab_status(p_tab_id, p_status, false);
	}
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_set_tab_status(int p_tab_id, const String &p_status, bool p_error) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	if (!tab.status_label) {
		return;
	}

	if (p_error) {
		tab.status_label->set_text(TTR("Error: ") + p_status);
	} else {
		tab.status_label->set_text(p_status.is_empty() ? TTR("Ready") : p_status);
	}
}

void UltimateAssistantPanel::_set_tab_loading_state(int p_tab_id, bool p_active, const String &p_base_text) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	tab.loading_indicator_active = p_active;
	if (!p_active) {
		_clear_loading_chat_indicator_for_tab(p_tab_id);
		if (tab.status_label && !tab.is_active) {
			tab.status_label->set_text(TTR("Ready"));
		}
		tab.loading_indicator_phase = 0;
		tab.next_loading_tick_msec = 0;
		tab.loading_chat_notice_emitted = false;
		tab.loading_indicator_text.clear();
		return;
	}

	tab.loading_indicator_phase = 0;
	tab.next_loading_tick_msec = OS::get_singleton()->get_ticks_msec();
	tab.loading_indicator_text = p_base_text.strip_edges();
	if (tab.loading_indicator_text.is_empty()) {
		tab.loading_indicator_text = TTR("Assistant is thinking");
	}
	tab.loading_chat_notice_emitted = false;
	_update_loading_chat_indicator_for_tab(p_tab_id);

	if (tab.status_label) {
		tab.status_label->set_text(TTR("Working..."));
	}
}

void UltimateAssistantPanel::_update_loading_chat_indicator_for_tab(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	if (!tab.loading_indicator_active || tab.assistant_stream_open || tab.command_stream_active) {
		_clear_loading_chat_indicator_for_tab(p_tab_id);
		return;
	}

	RichTextLabel *display = tab.chat_display;
	if (!display) {
		return;
	}

	if (!tab.loading_chat_indicator_active) {
		tab.loading_chat_indicator_prefix_len = tab.transcript.length();
		tab.loading_chat_indicator_active = true;
	}

	int spinner_index = tab.loading_indicator_phase % LOADING_SPINNER_FRAME_COUNT;
	String spinner = LOADING_SPINNER_FRAMES[spinner_index];
	tab.loading_indicator_phase = (tab.loading_indicator_phase + 1) % LOADING_SPINNER_FRAME_COUNT;

	String loading_text = tab.loading_indicator_text.strip_edges();
	if (loading_text.is_empty()) {
		loading_text = TTR("Assistant is thinking");
	}

	String line = "[i]" + _escape_bbcode_text(spinner + " " + loading_text + "...") + "[/i]\n\n";
	String base_transcript = tab.transcript.substr(0, tab.loading_chat_indicator_prefix_len);
	tab.transcript = base_transcript + line;
	display->set_text(tab.transcript);
	display->scroll_to_line(display->get_line_count());

	if (tab.status_label) {
		tab.status_label->set_text(spinner + " " + TTR("Working..."));
	}
}

void UltimateAssistantPanel::_clear_loading_chat_indicator_for_tab(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	if (!tab.loading_chat_indicator_active) {
		return;
	}

	if (tab.loading_chat_indicator_prefix_len < 0 || tab.loading_chat_indicator_prefix_len > tab.transcript.length()) {
		tab.loading_chat_indicator_prefix_len = tab.transcript.length();
	}

	tab.transcript = tab.transcript.substr(0, tab.loading_chat_indicator_prefix_len);
	if (tab.chat_display) {
		tab.chat_display->set_text(tab.transcript);
		tab.chat_display->scroll_to_line(tab.chat_display->get_line_count());
	}

	tab.loading_chat_indicator_active = false;
	tab.loading_chat_indicator_prefix_len = 0;
}

void UltimateAssistantPanel::_append_task_status_message(int p_tab_id, const String &p_status, bool p_force) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	String normalized = p_status.strip_edges().to_lower();
	if (normalized.is_empty()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	if (!p_force && tab.last_reported_task_status == normalized) {
		return;
	}
	tab.last_reported_task_status = normalized;

	_append_message(p_tab_id, TTR("System"), vformat("Gateway task: %s", normalized.replace("_", " ")));
}

void UltimateAssistantPanel::_start_command_stream_for_tab(int p_tab_id, const String &p_role, const String &p_content) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	String stream_role = p_role.strip_edges();
	if (stream_role.is_empty()) {
		stream_role = TTR("Assistant");
	}

	String remaining = p_content;
	if (remaining.is_empty()) {
		return;
	}
	_set_tab_loading_state(p_tab_id, false);

	tab.command_stream_active = false;
	tab.command_stream_role = stream_role;
	tab.command_stream_remaining.clear();
	tab.next_command_stream_tick_msec = 0;
	tab.assistant_stream_from_realtime = false;

	int first_chunk_len = MIN(COMMAND_STREAM_CHUNK_CHARS, remaining.length());
	String first_chunk = remaining.substr(0, first_chunk_len);
	String tail = remaining.substr(first_chunk_len, remaining.length() - first_chunk_len);
	bool finish = tail.is_empty();

	_append_stream_delta_for_tab(p_tab_id, stream_role, first_chunk, finish);
	if (finish) {
		_set_tab_loading_state(p_tab_id, false);
		_set_tab_status(p_tab_id, TTR("Ready"), false);
		return;
	}

	tab.command_stream_active = true;
	tab.command_stream_role = stream_role;
	tab.command_stream_remaining = tail;
	tab.next_command_stream_tick_msec = OS::get_singleton()->get_ticks_msec() + COMMAND_STREAM_TICK_MSEC;
}

Dictionary UltimateAssistantPanel::_build_project_map_payload(int p_tab_id) const {
	Dictionary project_map;
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	Dictionary version_info = Engine::get_singleton()->get_version_info();
	String engine_version = String(version_info.get("string", String("phoenix-editor"))).strip_edges();
	if (engine_version.is_empty()) {
		engine_version = "phoenix-editor";
	}

	String project_name = "phoenix-project";
	String main_scene = "res://";
	if (project_settings) {
		if (project_settings->has_setting("application/config/name")) {
			project_name = String(project_settings->get("application/config/name")).strip_edges();
		}
		if (project_settings->has_setting("application/run/main_scene")) {
			main_scene = String(project_settings->get("application/run/main_scene")).strip_edges();
		}
	}

	Dictionary resources;
	resources["audio"] = Array();
	resources["sprites"] = Array();
	resources["tilesets"] = Array();

	Dictionary extras;
	extras["tab_id"] = p_tab_id;
	extras["engine_version"] = engine_version;

	String hash_input = vformat("%s|%s|%s", project_name, main_scene, _now_iso8601_utc());

	project_map["name"] = project_name;
	project_map["godot_version"] = engine_version;
	project_map["main_scene"] = main_scene;
	project_map["scenes"] = Dictionary();
	project_map["scripts"] = Array();
	project_map["resources"] = resources;
	project_map["file_hash"] = "sha256:" + hash_input.sha256_text();
	project_map["extras"] = extras;

	return project_map;
}

Dictionary UltimateAssistantPanel::_build_task_context_payload(int p_tab_id) const {
	Dictionary context;
	context["current_file"] = "";
	context["scene_tree"] = Dictionary();
	Dictionary version_info = Engine::get_singleton()->get_version_info();
	String engine_version = String(version_info.get("string", String("unknown"))).strip_edges();
	if (engine_version.is_empty()) {
		engine_version = "unknown";
	}
	context["engine_version"] = engine_version;
	context["engine_version_major"] = int(version_info.get("major", 0));
	context["engine_version_minor"] = int(version_info.get("minor", 0));

	Array open_files;
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index >= 0 && tab_index < tabs.size()) {
		const ChatTab &tab = tabs[tab_index];
		if (tab.context_list) {
			int count = tab.context_list->get_item_count();
			for (int i = 0; i < count; i++) {
				Variant item_metadata = tab.context_list->get_item_metadata(i);
				if (item_metadata.get_type() == Variant::STRING) {
					open_files.push_back(String(item_metadata));
				}
			}
		}
	}
	context["open_files"] = open_files;

	Dictionary project_settings;
	project_settings["editor"] = "phoenix";
	project_settings["tab_id"] = p_tab_id;
	project_settings["engine_version"] = engine_version;

	Dictionary runtime_config = backend_adapter ? backend_adapter->get_runtime_config() : Dictionary();
	Dictionary enabled_tools;
	enabled_tools["godot_mcp_docs"] = runtime_config.has("tool_godot_mcp_docs_enabled") ? bool(runtime_config["tool_godot_mcp_docs_enabled"]) : true;
	enabled_tools["godot_mcp"] = runtime_config.has("tool_godot_mcp_enabled") ? bool(runtime_config["tool_godot_mcp_enabled"]) : true;
	enabled_tools["godot_copilot"] = runtime_config.has("tool_godot_copilot_enabled") ? bool(runtime_config["tool_godot_copilot_enabled"]) : true;
	enabled_tools["autonomous_agent_primitives"] = runtime_config.has("tool_autonomous_primitives_enabled") ? bool(runtime_config["tool_autonomous_primitives_enabled"]) : true;

	Dictionary tool_preferences;
	tool_preferences["mode"] = "auto";
	tool_preferences["enabled_tools"] = enabled_tools;

	Array selected_tools;
	if (tab_index >= 0 && tab_index < tabs.size()) {
		const ChatTab &tab = tabs[tab_index];
		if (tab.tool_mode_selector && tab.tool_mode_selector->get_selected() == 1) {
			tool_preferences["mode"] = "manual";
		}

		PackedStringArray selected_tool_ids = _collect_selected_tool_ids(tab);
		for (int i = 0; i < selected_tool_ids.size(); i++) {
			selected_tools.push_back(selected_tool_ids[i]);
		}
	}
	tool_preferences["selected_tools"] = selected_tools;

	Array available_tools;
	PackedStringArray enabled_tool_ids = _collect_enabled_tool_ids();
	for (int i = 0; i < enabled_tool_ids.size(); i++) {
		available_tools.push_back(enabled_tool_ids[i]);
	}
	tool_preferences["available_tools"] = available_tools;

	project_settings["tool_preferences"] = tool_preferences;
	context["project_settings"] = project_settings;

	return context;
}

String UltimateAssistantPanel::_resolve_runtime_user_id() const {
	if (!backend_adapter) {
		return "editor-user";
	}

	Dictionary runtime_config = backend_adapter->get_runtime_config();
	String actor_id = _sanitize_reviewer_id(String(runtime_config.get("actor_id", String())));
	if (!actor_id.is_empty()) {
		return actor_id;
	}

	return "editor-user";
}

bool UltimateAssistantPanel::_bootstrap_realtime_for_tab(int p_tab_id) {
	if (!frontend_runtime_adapter || !backend_adapter) {
		return false;
	}

	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return false;
	}
	ChatTab &tab = tabs.write[tab_index];

	if (tab.session_id.is_empty()) {
		return false;
	}

	String user_id = _resolve_runtime_user_id();
	Dictionary negotiation = frontend_runtime_adapter->negotiate_realtime(backend_adapter, tab.session_id, user_id);
	_log_request_context("realtime/negotiate", negotiation);

	if (!bool(negotiation.get("ok", false))) {
		tab.realtime_bootstrapped = false;
		tab.realtime_user_id = user_id;
		tab.realtime_url = String();
		tab.realtime_access_token = String();
		_disconnect_realtime_stream_for_tab(p_tab_id);
		String error = String(negotiation.get("error", String("Realtime negotiation unavailable."))).strip_edges();
		if (!error.is_empty()) {
			_append_message(p_tab_id, TTR("System"), vformat("Realtime unavailable; using polling fallback (%s).", error));
		}
		return false;
	}

	tab.realtime_bootstrapped = true;
	tab.realtime_user_id = user_id;
	tab.realtime_url = String(negotiation.get("realtime_url", String()));
	tab.realtime_access_token = String(negotiation.get("realtime_access_token", String()));

	bool realtime_connected = _connect_realtime_stream_for_tab(p_tab_id);
	if (!realtime_connected) {
		tab.realtime_bootstrapped = false;
		_append_message(p_tab_id, TTR("System"), TTR("Realtime stream unavailable; using polling fallback."));
	}

	Dictionary join_payload;
	join_payload["session_id"] = tab.session_id;
	join_payload["user_id"] = user_id;
	join_payload["groups"] = negotiation.get("realtime_groups", Array());
	Dictionary join_response = backend_adapter->realtime_join(join_payload);
	_log_request_context("realtime/join", join_response);

	if (!bool(join_response.get("ok", false))) {
		String join_error = String(join_response.get("error", String("Realtime join failed."))).strip_edges();
		if (!join_error.is_empty()) {
			_append_message(p_tab_id, TTR("System"), vformat("Realtime join fallback to polling (%s).", join_error));
		}
	}

	return realtime_connected;
}

bool UltimateAssistantPanel::_connect_realtime_stream_for_tab(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return false;
	}

	ChatTab &tab = tabs.write[tab_index];
	_disconnect_realtime_stream_for_tab(p_tab_id);

	String stream_url = _append_access_token_to_url(tab.realtime_url, tab.realtime_access_token);
	if (stream_url.is_empty()) {
		return false;
	}

	Ref<WebSocketPeer> realtime_peer = Ref<WebSocketPeer>(WebSocketPeer::create());
	if (realtime_peer.is_null()) {
		return false;
	}

	Error connect_error = realtime_peer->connect_to_url(stream_url);
	if (connect_error != OK) {
		return false;
	}

	tab.realtime_peer = realtime_peer;
	tab.realtime_stream_failed = false;
	return true;
}

void UltimateAssistantPanel::_disconnect_realtime_stream_for_tab(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	if (tab.realtime_peer.is_valid()) {
		WebSocketPeer::State state = tab.realtime_peer->get_ready_state();
		if (state == WebSocketPeer::STATE_CONNECTING || state == WebSocketPeer::STATE_OPEN || state == WebSocketPeer::STATE_CLOSING) {
			tab.realtime_peer->close(1000, "panel_close");
		}
	}
	tab.realtime_peer.unref();
	tab.realtime_stream_failed = false;
}

void UltimateAssistantPanel::_poll_realtime_stream_for_tab(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	if (!tab.realtime_peer.is_valid()) {
		return;
	}

	tab.realtime_peer->poll();
	WebSocketPeer::State state = tab.realtime_peer->get_ready_state();
	if (state == WebSocketPeer::STATE_CLOSED) {
		if (!tab.realtime_stream_failed) {
			tab.realtime_stream_failed = true;
			int close_code = tab.realtime_peer->get_close_code();
			String close_reason = tab.realtime_peer->get_close_reason().strip_edges();
			String detail = close_reason.is_empty()
					? vformat("Realtime stream closed (code %d).", close_code)
					: vformat("Realtime stream closed (code %d: %s).", close_code, close_reason);
			_append_message(p_tab_id, TTR("System"), detail + " " + TTR("Falling back to status polling."));
		}
		tab.realtime_peer.unref();
		return;
	}

	if (state != WebSocketPeer::STATE_OPEN) {
		return;
	}

	Array realtime_events;
	int available_packets = tab.realtime_peer->get_available_packet_count();
	for (int i = 0; i < available_packets; i++) {
		const uint8_t *packet_data = nullptr;
		int packet_size = 0;
		Error packet_error = tab.realtime_peer->get_packet(&packet_data, packet_size);
		if (packet_error != OK || packet_data == nullptr || packet_size <= 0) {
			continue;
		}

		String payload_text = String::utf8((const char *)packet_data, packet_size);
		Variant payload = JSON::parse_string(payload_text);
		if (payload.get_type() == Variant::NIL && payload_text.strip_edges() != "null") {
			continue;
		}

		_collect_realtime_event_payloads(payload, realtime_events);
	}

	if (!realtime_events.is_empty()) {
		_apply_realtime_events_for_tab(p_tab_id, realtime_events, false);
	}
}

void UltimateAssistantPanel::_apply_realtime_events_for_tab(int p_tab_id, const Array &p_events, bool p_refresh_status) {
	if (!frontend_runtime_adapter) {
		return;
	}

	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];

	for (int i = 0; i < p_events.size(); i++) {
		Variant event_v = p_events[i];
		if (event_v.get_type() != Variant::DICTIONARY) {
			continue;
		}

		Dictionary event = event_v;
		Dictionary mapped = frontend_runtime_adapter->map_realtime_event(event, tab.session_id);
		if (!bool(mapped.get("handled", false))) {
			continue;
		}

		int seq = int(mapped.get("seq", -1));
		if (seq >= 0) {
			if (seq <= tab.last_realtime_seq) {
				continue;
			}
			tab.last_realtime_seq = seq;
		}

		String chat_delta = String(mapped.get("chat_delta", String()));
		String thinking_delta = String(mapped.get("thinking_delta", String()));
		const bool thinking_stream_enabled = _is_thinking_stream_enabled();
		if (thinking_stream_enabled && !thinking_delta.is_empty()) {
			tab.suppress_next_chat_message = true;
			tab.command_stream_active = false;
			tab.command_stream_remaining.clear();
			tab.command_stream_role.clear();
			tab.next_command_stream_tick_msec = 0;
			_set_tab_loading_state(p_tab_id, false);
			_append_thinking_delta_for_tab(p_tab_id, thinking_delta, false);
		}

		if (thinking_stream_enabled && bool(mapped.get("thinking_done", false))) {
			_append_thinking_delta_for_tab(p_tab_id, String(), true);
		}

		if (!chat_delta.is_empty()) {
			tab.suppress_next_chat_message = true;
			tab.command_stream_active = false;
			tab.command_stream_remaining.clear();
			tab.command_stream_role.clear();
			tab.next_command_stream_tick_msec = 0;
			if (tab.assistant_stream_open && !tab.assistant_stream_from_realtime && tab.assistant_stream_prefix_len >= 0 && tab.assistant_stream_prefix_len <= tab.transcript.length()) {
				tab.transcript = tab.transcript.substr(0, tab.assistant_stream_prefix_len);
				if (tab.chat_display) {
					tab.chat_display->set_text(tab.transcript);
					tab.chat_display->scroll_to_line(tab.chat_display->get_line_count());
				}
				tab.assistant_stream_open = false;
				tab.assistant_stream_text.clear();
				tab.assistant_stream_prefix_len = 0;
			}
			tab.assistant_stream_from_realtime = true;
			_set_tab_loading_state(p_tab_id, false);
			String chat_role = String(mapped.get("chat_role", String("Assistant")));
			_append_stream_delta_for_tab(p_tab_id, chat_role, chat_delta, false);
		}

		if (bool(mapped.get("chat_done", false))) {
			_append_stream_delta_for_tab(p_tab_id, String(), String(), true);
			_set_tab_loading_state(p_tab_id, false);
		}

		String message = String(mapped.get("message", String())).strip_edges();
		if (!message.is_empty()) {
			_append_message(p_tab_id, TTR("Realtime"), message);
		}

		String event_name = String(mapped.get("event", String())).strip_edges();
		if (event_name == "lock.conflict") {
			_append_lock_snapshot_for_tab(p_tab_id, TTR("Realtime lock conflict detected."));
		}

		if (bool(mapped.get("requires_resync", false))) {
			Dictionary synthetic_conflict;
			synthetic_conflict["error"] = message.is_empty() ? String("Realtime requested session resync.") : message;
			_apply_conflict_state(p_tab_id, synthetic_conflict);
			continue;
		}

		if (bool(mapped.get("requires_status_refresh", false))) {
			String plan_id = String(mapped.get("plan_id", String())).strip_edges();
			if (!plan_id.is_empty()) {
				tab.last_plan_id = plan_id;
				tab.status_poll_active = true;
				tab.next_status_poll_msec = OS::get_singleton()->get_ticks_msec() + TASK_STATUS_LIVE_POLL_INITIAL_DELAY_MSEC;
			}
			if (tab.resync_button) {
				tab.resync_button->set_visible(true);
			}
			if (p_refresh_status) {
				_set_tab_status(p_tab_id, TTR("Realtime update received; refreshing task status..."), false);
			}
		}
	}
}

void UltimateAssistantPanel::_append_lock_snapshot_for_tab(int p_tab_id, const String &p_reason) {
	if (!backend_adapter) {
		return;
	}

	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	const ChatTab &tab = tabs[tab_index];

	Dictionary response = backend_adapter->list_locks(tab.session_id);
	_log_request_context("locks/list", response);

	if (!bool(response.get("ok", false))) {
		String detail = String(response.get("error", String("Unable to fetch active locks."))).strip_edges();
		if (detail.is_empty()) {
			detail = "Unable to fetch active locks.";
		}
		_append_message(p_tab_id, TTR("System"), vformat("%s %s", p_reason.is_empty() ? String() : p_reason + " ", detail));
		return;
	}

	Variant body_v = response.get("body", Variant());
	if (body_v.get_type() != Variant::DICTIONARY) {
		return;
	}

	Dictionary body = body_v;
	Variant locks_v = body.get("locks", Variant());
	if (locks_v.get_type() != Variant::ARRAY) {
		return;
	}

	Array locks = locks_v;
	if (locks.is_empty()) {
		_append_message(p_tab_id, TTR("System"), TTR("No active locks reported for this session."));
		return;
	}

	int preview_count = MIN(3, locks.size());
	String summary;
	for (int i = 0; i < preview_count; i++) {
		Variant lock_v = locks[i];
		if (lock_v.get_type() != Variant::DICTIONARY) {
			continue;
		}

		Dictionary lock = lock_v;
		String lock_id = String(lock.get("lock_id", lock.get("lockId", String()))).strip_edges();
		String resource_path = String(lock.get("resource_path", lock.get("resourcePath", String()))).strip_edges();
		String holder_display = String(lock.get("holder_display_name", lock.get("holderDisplayName", String()))).strip_edges();
		String holder_id = String(lock.get("holder_id", lock.get("holderId", String()))).strip_edges();

		String line = resource_path.is_empty() ? lock_id : resource_path;
		if (line.is_empty()) {
			line = String("<unknown>");
		}
		if (!holder_display.is_empty()) {
			line += vformat(" (holder: %s)", holder_display);
		} else if (!holder_id.is_empty()) {
			line += vformat(" (holder: %s)", holder_id);
		}

		if (!summary.is_empty()) {
			summary += "; ";
		}
		summary += line;
	}

	String intro = p_reason.is_empty() ? String() : p_reason + " ";
	_append_message(
			p_tab_id,
			TTR("System"),
			vformat("%sActive locks (%d): %s", intro, locks.size(), summary.is_empty() ? String("<details unavailable>") : summary));

	if (locks.size() > preview_count) {
		_append_message(p_tab_id, TTR("System"), vformat("%d additional locks omitted.", locks.size() - preview_count));
	}
}

bool UltimateAssistantPanel::_start_session_for_tab(int p_tab_id, bool p_force_resync) {
	if (!backend_adapter) {
		_set_tab_status(p_tab_id, TTR("Backend adapter unavailable."), true);
		return false;
	}

	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return false;
	}
	ChatTab &tab = tabs.write[tab_index];

	if (!p_force_resync && !tab.session_id.is_empty() && !tab.idempotency_key.is_empty()) {
		if (!tab.realtime_peer.is_valid()) {
			_bootstrap_realtime_for_tab(p_tab_id);
		}
		return true;
	}

	if (tab.session_id.is_empty()) {
		tab.session_id = _id_with_prefix(vformat("sess-%d", p_tab_id));
	}

	String idempotency_key = _id_with_prefix(vformat("idem-%d", p_tab_id));

	Dictionary payload;
	payload["schema_version"] = "v1";
	payload["event"] = "session_start";
	payload["session_id"] = tab.session_id;
	payload["idempotency_key"] = idempotency_key;
	payload["sent_at"] = _now_iso8601_utc();
	payload["project_map"] = _build_project_map_payload(p_tab_id);

	Dictionary response = backend_adapter->start_session(payload);
	_log_request_context("session/start", response);

	if (!bool(response.get("ok", false))) {
		if (int(response.get("status_code", 0)) == HTTPClient::RESPONSE_CONFLICT) {
			_apply_conflict_state(p_tab_id, response);
		} else {
			String error = String(response.get("error", TTR("Session start failed.")));
			_set_tab_status(p_tab_id, error, true);
			_append_message(p_tab_id, TTR("System"), vformat("Session start failed: %s", error));
		}
		return false;
	}

	tab.idempotency_key = idempotency_key;
	_clear_conflict_state(p_tab_id);
	_bootstrap_realtime_for_tab(p_tab_id);
	return true;
}

void UltimateAssistantPanel::_request_task_for_tab(int p_tab_id, const String &p_user_input) {
	if (!backend_adapter) {
		_set_tab_status(p_tab_id, TTR("Backend adapter unavailable."), true);
		return;
	}

	String request_text = p_user_input.strip_edges();
	if (request_text.is_empty()) {
		return;
	}

	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	tab.suppress_next_chat_message = false;
	tab.command_stream_active = false;
	tab.command_stream_role.clear();
	tab.command_stream_remaining.clear();
	tab.next_command_stream_tick_msec = 0;
	tab.last_reported_task_status.clear();
	tab.assistant_stream_text.clear();
	tab.assistant_stream_prefix_len = 0;
	if (tab.thinking_stream_open) {
		_append_thinking_delta_for_tab(p_tab_id, String(), true);
	}
	if (tab.assistant_stream_open) {
		_append_stream_delta_for_tab(p_tab_id, String(), String(), true);
	}

	if (tab.session_id.is_empty()) {
		tab.session_id = _id_with_prefix(vformat("sess-%d", p_tab_id));
	}

	bool should_start_session = tab.idempotency_key.is_empty();
	String idempotency_key = tab.idempotency_key;

	Dictionary session_payload;
	if (should_start_session) {
		idempotency_key = _id_with_prefix(vformat("idem-%d", p_tab_id));
		session_payload["schema_version"] = "v1";
		session_payload["event"] = "session_start";
		session_payload["session_id"] = tab.session_id;
		session_payload["idempotency_key"] = idempotency_key;
		session_payload["sent_at"] = _now_iso8601_utc();
		session_payload["project_map"] = _build_project_map_payload(p_tab_id);
	}

	tab.task_counter += 1;
	String task_id = vformat("task-%d-%d", p_tab_id, tab.task_counter);

	Dictionary payload;
	payload["schema_version"] = "v1";
	payload["session_id"] = tab.session_id;
	payload["task_id"] = task_id;
	payload["user_input"] = request_text;
	payload["mode"] = _readable_mode_from_selector(tab.mode_selector);
	payload["submitted_at"] = _now_iso8601_utc();
	payload["project_context"] = _build_task_context_payload(p_tab_id);

	tab.request_generation += 1;
	tab.task_request_in_flight = true;

	_set_tab_busy(p_tab_id, true, should_start_session ? TTR("Starting session...") : TTR("Requesting task plan..."));

	AsyncTaskRequestJob *job = memnew(AsyncTaskRequestJob);
	job->tab_id = p_tab_id;
	job->request_generation = tab.request_generation;
	job->should_start_session = should_start_session;
	job->session_id = tab.session_id;
	job->idempotency_key = idempotency_key;
	job->runtime_config = backend_adapter->get_runtime_config();
	job->session_payload = session_payload;
	job->task_payload = payload;

	_enqueue_async_task_request(job);
}

void UltimateAssistantPanel::_enqueue_async_task_request(AsyncTaskRequestJob *p_job) {
	ERR_FAIL_NULL(p_job);

	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	if (!pool) {
		_run_async_task_request_job(p_job);
		_handle_async_task_request_completion(p_job);
		memdelete(p_job);
		return;
	}

	int64_t task_id = pool->add_template_task(this, &UltimateAssistantPanel::_run_async_task_request_job, p_job, true, "UltimateAIChatTaskRequest");
	if (task_id == WorkerThreadPool::INVALID_TASK_ID) {
		_run_async_task_request_job(p_job);
		_handle_async_task_request_completion(p_job);
		memdelete(p_job);
		return;
	}

	PendingAsyncTaskRequest pending;
	pending.task_id = task_id;
	pending.job = p_job;
	pending_task_requests.push_back(pending);
}

void UltimateAssistantPanel::_run_async_task_request_job(AsyncTaskRequestJob *p_job) {
	ERR_FAIL_NULL(p_job);

	UltimateAIBackendContractAdapter request_adapter;
	request_adapter.apply_runtime_config(p_job->runtime_config);

	if (p_job->should_start_session) {
		p_job->session_response = request_adapter.start_session(p_job->session_payload);
		if (!bool(p_job->session_response.get("ok", false))) {
			return;
		}
	}

	p_job->task_response = request_adapter.request_task(p_job->task_payload);
}

void UltimateAssistantPanel::_pump_async_task_requests() {
	if (pending_task_requests.is_empty()) {
		return;
	}

	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	if (!pool) {
		return;
	}

	for (int i = pending_task_requests.size() - 1; i >= 0; i--) {
		const PendingAsyncTaskRequest &pending = pending_task_requests[i];
		if (pending.task_id == WorkerThreadPool::INVALID_TASK_ID || pending.job == nullptr) {
			pending_task_requests.remove_at(i);
			continue;
		}

		if (!pool->is_task_completed(pending.task_id)) {
			continue;
		}

		pool->wait_for_task_completion(pending.task_id);
		AsyncTaskRequestJob *job = pending.job;
		pending_task_requests.remove_at(i);
		_handle_async_task_request_completion(job);
		memdelete(job);
	}
}

void UltimateAssistantPanel::_handle_async_task_request_completion(AsyncTaskRequestJob *p_job) {
	ERR_FAIL_NULL(p_job);

	int tab_index = _find_tab_index_by_id(p_job->tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	if (tab.request_generation != p_job->request_generation) {
		return;
	}

	tab.task_request_in_flight = false;

	if (p_job->should_start_session) {
		_log_request_context("session/start", p_job->session_response);
		if (!bool(p_job->session_response.get("ok", false))) {
			if (int(p_job->session_response.get("status_code", 0)) == HTTPClient::RESPONSE_CONFLICT) {
				_apply_conflict_state(p_job->tab_id, p_job->session_response);
			} else {
				String error = String(p_job->session_response.get("error", TTR("Session start failed.")));
				_set_tab_status(p_job->tab_id, error, true);
				_append_message(p_job->tab_id, TTR("System"), vformat("Session start failed: %s", error));
			}
			_set_tab_loading_state(p_job->tab_id, false);
			_set_tab_busy(p_job->tab_id, false);
			return;
		}

		tab.session_id = p_job->session_id;
		tab.idempotency_key = p_job->idempotency_key;
		_clear_conflict_state(p_job->tab_id);
	}

	Dictionary response = p_job->task_response;
	_log_request_context("task/request", response);

	if (!bool(response.get("ok", false))) {
		if (int(response.get("status_code", 0)) == HTTPClient::RESPONSE_CONFLICT) {
			_apply_conflict_state(p_job->tab_id, response);
		} else {
			String error = String(response.get("error", TTR("Task request failed.")));
			_set_tab_status(p_job->tab_id, error, true);
			_append_message(p_job->tab_id, TTR("System"), vformat("Task request failed: %s", error));
		}
		_set_tab_loading_state(p_job->tab_id, false);
		_set_tab_busy(p_job->tab_id, false);
		return;
	}

	_clear_conflict_state(p_job->tab_id);

	Variant body_v = response.get("body", Variant());
	if (body_v.get_type() != Variant::DICTIONARY) {
		_set_tab_status(p_job->tab_id, TTR("Invalid backend response."), true);
		_append_message(p_job->tab_id, TTR("System"), TTR("Backend response did not contain a valid JSON object."));
		_set_tab_loading_state(p_job->tab_id, false);
		_set_tab_busy(p_job->tab_id, false);
		return;
	}

	Dictionary body = body_v;
	String response_event = String(body.get("event", String())).to_lower();
	if ((body.has("accepted") || response_event == "task_queued_ack") && !_is_valid_task_request_accepted_payload(body)) {
		_set_tab_status(p_job->tab_id, TTR("Task request response does not match Interface contract."), true);
		_append_message(p_job->tab_id, TTR("System"), TTR("Gateway returned an invalid task_request response shape (expected task_queued_ack)."));
		_set_tab_loading_state(p_job->tab_id, false);
		_set_tab_busy(p_job->tab_id, false);
		return;
	}

	bool keep_waiting_for_result = false;

	if (body.has("actions")) {
		Array actions = body.get("actions", Array());
		bool batch_requires_approval = bool(body.get("requires_approval", false));

		Array immediate_commands;
		Array approval_actions;
		for (int i = 0; i < actions.size(); i++) {
			Variant action_v = actions[i];
			if (action_v.get_type() != Variant::DICTIONARY) {
				continue;
			}

			Dictionary action = action_v;
			bool action_requires_approval = action.has("requires_approval")
					? bool(action.get("requires_approval", false))
					: batch_requires_approval;
			if (action_requires_approval) {
				approval_actions.push_back(action);
				continue;
			}

			if (action.has("command") && action["command"].get_type() == Variant::DICTIONARY) {
				immediate_commands.push_back(action["command"]);
			}
		}

		if (!immediate_commands.is_empty()) {
			_execute_commands_for_tab(p_job->tab_id, immediate_commands);
		}

		if (!approval_actions.is_empty()) {
			Dictionary approval_batch = body;
			approval_batch["actions"] = approval_actions;
			approval_batch["requires_approval"] = true;
			_show_approval_batch(p_job->tab_id, approval_batch);

			String summary = String(approval_batch.get("approval_summary", String()));
			_append_message(p_job->tab_id, TTR("System"), summary.is_empty() ? TTR("Approval required before execution.") : summary);
		} else {
			_clear_approval_batch(p_job->tab_id);
		}
	} else if (body.has("commands") && body["commands"].get_type() == Variant::ARRAY) {
		_execute_commands_for_tab(p_job->tab_id, body["commands"]);
	} else if (bool(body.get("accepted", false)) && body.has("plan_id")) {
		String plan_id = String(body.get("plan_id", String()));
		tab.last_plan_id = plan_id;
		tab.last_realtime_seq = -1;
		tab.suppress_next_chat_message = false;
		tab.status_poll_active = true;
		tab.next_status_poll_msec = OS::get_singleton()->get_ticks_msec() + TASK_STATUS_LIVE_POLL_INITIAL_DELAY_MSEC;
		_set_tab_status(p_job->tab_id, TTR("Working..."), false);
		_set_tab_loading_state(p_job->tab_id, true, TTR("Assistant is thinking"));
		keep_waiting_for_result = true;
	}

	if (keep_waiting_for_result) {
		return;
	}

	_set_tab_loading_state(p_job->tab_id, false);
	_set_tab_status(p_job->tab_id, TTR("Ready"), false);
	_set_tab_busy(p_job->tab_id, false);
}

void UltimateAssistantPanel::_drain_async_task_requests() {
	if (pending_task_requests.is_empty()) {
		return;
	}

	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	for (int i = 0; i < pending_task_requests.size(); i++) {
		const PendingAsyncTaskRequest &pending = pending_task_requests[i];
		if (pending.job == nullptr) {
			continue;
		}
		if (pool && pending.task_id != WorkerThreadPool::INVALID_TASK_ID) {
			pool->wait_for_task_completion(pending.task_id);
		}
		memdelete(pending.job);
	}
	pending_task_requests.clear();
}

void UltimateAssistantPanel::_enqueue_async_task_status_poll(AsyncTaskStatusPollJob *p_job) {
	ERR_FAIL_NULL(p_job);

	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	if (!pool) {
		_run_async_task_status_poll_job(p_job);
		_handle_async_task_status_poll_completion(p_job);
		memdelete(p_job);
		return;
	}

	int64_t task_id = pool->add_template_task(this, &UltimateAssistantPanel::_run_async_task_status_poll_job, p_job, true, "UltimateAIStatusPoll");
	if (task_id == WorkerThreadPool::INVALID_TASK_ID) {
		_run_async_task_status_poll_job(p_job);
		_handle_async_task_status_poll_completion(p_job);
		memdelete(p_job);
		return;
	}

	PendingAsyncTaskStatusPoll pending;
	pending.task_id = task_id;
	pending.job = p_job;
	pending_status_poll_requests.push_back(pending);
}

void UltimateAssistantPanel::_run_async_task_status_poll_job(AsyncTaskStatusPollJob *p_job) {
	ERR_FAIL_NULL(p_job);

	UltimateAIBackendContractAdapter request_adapter;
	request_adapter.apply_runtime_config(p_job->runtime_config);
	p_job->response = request_adapter.get_task_status(p_job->plan_id);
}

void UltimateAssistantPanel::_pump_async_task_status_polls() {
	if (pending_status_poll_requests.is_empty()) {
		return;
	}

	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	if (!pool) {
		return;
	}

	for (int i = pending_status_poll_requests.size() - 1; i >= 0; i--) {
		const PendingAsyncTaskStatusPoll &pending = pending_status_poll_requests[i];
		if (pending.task_id == WorkerThreadPool::INVALID_TASK_ID || pending.job == nullptr) {
			pending_status_poll_requests.remove_at(i);
			continue;
		}

		if (!pool->is_task_completed(pending.task_id)) {
			continue;
		}

		pool->wait_for_task_completion(pending.task_id);
		AsyncTaskStatusPollJob *job = pending.job;
		pending_status_poll_requests.remove_at(i);
		_handle_async_task_status_poll_completion(job);
		memdelete(job);
	}
}

void UltimateAssistantPanel::_handle_async_task_status_poll_completion(AsyncTaskStatusPollJob *p_job) {
	ERR_FAIL_NULL(p_job);

	int tab_index = _find_tab_index_by_id(p_job->tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	if (tab.status_poll_generation != p_job->poll_generation) {
		return;
	}

	tab.status_poll_request_in_flight = false;
	if (tab.has_conflict || tab.last_plan_id != p_job->plan_id) {
		return;
	}

	bool is_live_poll = tab.status_poll_active && !p_job->one_shot_refresh;
	bool is_manual_refresh = p_job->one_shot_refresh;
	if (!is_live_poll && !is_manual_refresh) {
		return;
	}

	_log_request_context("task/status", p_job->response);
	bool terminal = _handle_task_status_response_for_tab(p_job->tab_id, p_job->plan_id, p_job->response);
	if (is_manual_refresh) {
		if (!terminal) {
			_append_message(p_job->tab_id, TTR("System"), TTR("Gateway task is still queued or planning."));
		}
		_set_tab_busy(p_job->tab_id, false);
		return;
	}

	if (terminal) {
		_finalize_task_poll_terminal_state(p_job->tab_id);
	}
}

void UltimateAssistantPanel::_drain_async_task_status_polls() {
	if (pending_status_poll_requests.is_empty()) {
		return;
	}

	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	for (int i = 0; i < pending_status_poll_requests.size(); i++) {
		const PendingAsyncTaskStatusPoll &pending = pending_status_poll_requests[i];
		if (pending.job == nullptr) {
			continue;
		}
		if (pool && pending.task_id != WorkerThreadPool::INVALID_TASK_ID) {
			pool->wait_for_task_completion(pending.task_id);
		}
		memdelete(pending.job);
	}
	pending_status_poll_requests.clear();
}

void UltimateAssistantPanel::_enqueue_async_approval(AsyncApprovalJob *p_job) {
	ERR_FAIL_NULL(p_job);

	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	if (!pool) {
		_run_async_approval_job(p_job);
		_handle_async_approval_completion(p_job);
		memdelete(p_job);
		return;
	}

	int64_t task_id = pool->add_template_task(this, &UltimateAssistantPanel::_run_async_approval_job, p_job, true, "UltimateAIApproval");
	if (task_id == WorkerThreadPool::INVALID_TASK_ID) {
		_run_async_approval_job(p_job);
		_handle_async_approval_completion(p_job);
		memdelete(p_job);
		return;
	}

	PendingAsyncApproval pending;
	pending.task_id = task_id;
	pending.job = p_job;
	pending_approval_requests.push_back(pending);
}

void UltimateAssistantPanel::_run_async_approval_job(AsyncApprovalJob *p_job) {
	ERR_FAIL_NULL(p_job);

	UltimateAIBackendContractAdapter request_adapter;
	request_adapter.apply_runtime_config(p_job->runtime_config);
	p_job->response = request_adapter.submit_approval(p_job->plan_id, p_job->payload);
}

void UltimateAssistantPanel::_pump_async_approvals() {
	if (pending_approval_requests.is_empty()) {
		return;
	}

	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	if (!pool) {
		return;
	}

	for (int i = pending_approval_requests.size() - 1; i >= 0; i--) {
		const PendingAsyncApproval &pending = pending_approval_requests[i];
		if (pending.task_id == WorkerThreadPool::INVALID_TASK_ID || pending.job == nullptr) {
			pending_approval_requests.remove_at(i);
			continue;
		}

		if (!pool->is_task_completed(pending.task_id)) {
			continue;
		}

		pool->wait_for_task_completion(pending.task_id);
		AsyncApprovalJob *job = pending.job;
		pending_approval_requests.remove_at(i);
		_handle_async_approval_completion(job);
		memdelete(job);
	}
}

void UltimateAssistantPanel::_handle_async_approval_completion(AsyncApprovalJob *p_job) {
	ERR_FAIL_NULL(p_job);

	int tab_index = _find_tab_index_by_id(p_job->tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	if (tab.approval_generation != p_job->approval_generation) {
		return;
	}

	tab.approval_request_in_flight = false;
	Dictionary response = p_job->response;
	_log_request_context("task/approval", response);

	if (!bool(response.get("ok", false))) {
		if (int(response.get("status_code", 0)) == HTTPClient::RESPONSE_CONFLICT) {
			_apply_conflict_state(p_job->tab_id, response);
		} else {
			String error = String(response.get("error", TTR("Approval request failed.")));
			_set_tab_status(p_job->tab_id, error, true);
			_append_message(p_job->tab_id, TTR("System"), vformat("Approval failed: %s", error));
		}
		_set_tab_busy(p_job->tab_id, false);
		return;
	}

	_clear_conflict_state(p_job->tab_id);
	_clear_approval_batch(p_job->tab_id);

	Variant body_v = response.get("body", Variant());
	if (body_v.get_type() == Variant::DICTIONARY) {
		Dictionary body = body_v;
		if (body.has("commands") && body["commands"].get_type() == Variant::ARRAY) {
			_execute_commands_for_tab(p_job->tab_id, body["commands"]);
		}
	}

	_set_tab_status(p_job->tab_id, TTR("Approval processed."), false);
	_set_tab_busy(p_job->tab_id, false);
}

void UltimateAssistantPanel::_drain_async_approvals() {
	if (pending_approval_requests.is_empty()) {
		return;
	}

	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	for (int i = 0; i < pending_approval_requests.size(); i++) {
		const PendingAsyncApproval &pending = pending_approval_requests[i];
		if (pending.job == nullptr) {
			continue;
		}
		if (pool && pending.task_id != WorkerThreadPool::INVALID_TASK_ID) {
			pool->wait_for_task_completion(pending.task_id);
		}
		memdelete(pending.job);
	}
	pending_approval_requests.clear();
}

void UltimateAssistantPanel::_apply_conflict_state(int p_tab_id, const Dictionary &p_response) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	tab.has_conflict = true;
	tab.status_poll_active = false;
	tab.status_poll_request_in_flight = false;
	tab.approval_request_in_flight = false;
	tab.next_status_poll_msec = 0;
	_set_tab_loading_state(p_tab_id, false);

	if (tab.resync_button) {
		tab.resync_button->set_visible(true);
	}

	String detail = String(p_response.get("error", TTR("Session conflict.")));
	_set_tab_status(p_tab_id, vformat("Conflict (409): %s", detail), true);
	_append_message(p_tab_id, TTR("System"), vformat("Conflict detected (%s). Use Resync to refresh session state.", detail));
	_append_lock_snapshot_for_tab(p_tab_id, TTR("Conflict details:"));
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_clear_conflict_state(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	tab.has_conflict = false;
	tab.status_poll_active = false;
	tab.next_status_poll_msec = 0;
	if (tab.resync_button) {
		tab.resync_button->set_visible(false);
	}
}

void UltimateAssistantPanel::_show_approval_batch(int p_tab_id, const Dictionary &p_batch) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];

	tab.last_plan_id = String(p_batch.get("plan_id", String()));
	tab.pending_action_ids.clear();
	tab.pending_actions.clear();

	if (tab.approval_list) {
		tab.approval_list->clear();
	}

	Array actions = p_batch.get("actions", Array());
	for (int i = 0; i < actions.size(); i++) {
		Variant action_v = actions[i];
		if (action_v.get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary action = action_v;
		String action_id = String(action.get("action_id", String()));
		if (!action_id.is_empty()) {
			tab.pending_action_ids.push_back(action_id);
		}
		tab.pending_actions.push_back(action);

		if (tab.approval_list) {
			Dictionary command = action.get("command", Dictionary());
			String action_name = String(command.get("action", "unknown"));
			String risk = String(action.get("risk_level", "unknown"));
			String label = vformat("%s  |  %s  |  risk=%s", action_id, action_name, risk);
			int idx = tab.approval_list->add_item(label);
			tab.approval_list->set_item_metadata(idx, action_id);
		}
	}

	if (tab.approval_label) {
		String summary = String(p_batch.get("approval_summary", String()));
		if (summary.is_empty()) {
			summary = TTR("Pending approval");
		}
		tab.approval_label->set_text(summary);
	}

	if (tab.approval_section) {
		tab.approval_section->set_visible(!tab.pending_action_ids.is_empty());
	}
	_refresh_hub();
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_clear_approval_batch(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	tab.pending_action_ids.clear();
	tab.pending_actions.clear();
	tab.last_plan_id = "";
	tab.approval_request_in_flight = false;

	if (tab.approval_list) {
		tab.approval_list->clear();
	}
	if (tab.approval_section) {
		tab.approval_section->set_visible(false);
	}
	_refresh_hub();
	_broadcast_shared_state();
}

bool UltimateAssistantPanel::_handle_task_status_response_for_tab(int p_tab_id, const String &p_plan_id, const Dictionary &p_response) {
	if (bool(p_response.get("ok", false))) {
		Variant body_v = p_response.get("body", Variant());
		if (body_v.get_type() == Variant::DICTIONARY) {
			Dictionary body = body_v;
			Variant realtime_events_v = body.get("realtime_events", Variant());
			if (realtime_events_v.get_type() == Variant::ARRAY) {
				_apply_realtime_events_for_tab(p_tab_id, realtime_events_v, true);
			}

			int tab_index = _find_tab_index_by_id(p_tab_id);
			if (tab_index >= 0 && tab_index < tabs.size()) {
				if (tabs[tab_index].has_conflict) {
					return true;
				}
			}

			if (!_is_valid_task_status_payload(body, p_plan_id)) {
				_set_tab_status(p_tab_id, TTR("Task status response does not match Interface contract."), true);
				_append_message(p_tab_id, TTR("System"), TTR("Gateway returned an invalid task_status response shape."));
				return true;
			}

			String task_status = String(body.get("status", String())).to_lower();
			if (!task_status.is_empty()) {
				_set_tab_status(p_tab_id, TTR("Working..."), false);
				if (task_status == "error") {
					_append_message(p_tab_id, TTR("System"), TTR("Gateway reported task execution error."));
					return true;
				}
			}

			if (body.has("commands") && body["commands"].get_type() == Variant::ARRAY) {
				_execute_commands_for_tab(p_tab_id, body["commands"]);
				if (task_status == "done") {
					return true;
				}
			}

			Variant proposed_batch_v = body.get("proposed_action_batch", Variant());
			if (proposed_batch_v.get_type() == Variant::DICTIONARY) {
				Dictionary proposed_batch = proposed_batch_v;
				if (!proposed_batch.has("plan_id")) {
					proposed_batch["plan_id"] = p_plan_id;
				}

				Array actions = proposed_batch.get("actions", Array());
				bool batch_requires_approval = bool(proposed_batch.get("requires_approval", false));

				Array immediate_commands;
				Array approval_actions;
				for (int i = 0; i < actions.size(); i++) {
					Variant action_v = actions[i];
					if (action_v.get_type() != Variant::DICTIONARY) {
						continue;
					}

					Dictionary action = action_v;
					bool action_requires_approval = action.has("requires_approval")
							? bool(action.get("requires_approval", false))
							: batch_requires_approval;
					if (action_requires_approval) {
						approval_actions.push_back(action);
						continue;
					}

					if (action.has("command") && action["command"].get_type() == Variant::DICTIONARY) {
						immediate_commands.push_back(action["command"]);
					}
				}

				if (!immediate_commands.is_empty()) {
					_execute_commands_for_tab(p_tab_id, immediate_commands);
				}

				if (!approval_actions.is_empty()) {
					Dictionary approval_batch = proposed_batch;
					approval_batch["actions"] = approval_actions;
					approval_batch["requires_approval"] = true;
					_show_approval_batch(p_tab_id, approval_batch);

					String summary = String(approval_batch.get("approval_summary", String()));
					_append_message(p_tab_id, TTR("System"), summary.is_empty() ? TTR("Approval required before execution.") : summary);
					return true;
				}

				_clear_approval_batch(p_tab_id);
				if (task_status == "done") {
					return true;
				}
				_set_tab_status(p_tab_id, TTR("Working..."), false);
				return false;
			}

			if (task_status == "done") {
				return true;
			}

			_set_tab_status(p_tab_id, TTR("Working..."), false);
			return false;
		}

		_set_tab_status(p_tab_id, TTR("Task status response does not match Interface contract."), true);
		_append_message(p_tab_id, TTR("System"), TTR("Gateway returned an invalid task_status response shape."));
		return true;
	}

	int status_code = int(p_response.get("status_code", 0));
	if (status_code == HTTPClient::RESPONSE_CONFLICT) {
		_apply_conflict_state(p_tab_id, p_response);
		return true;
	}
	if (status_code != HTTPClient::RESPONSE_NOT_FOUND) {
		String error = String(p_response.get("error", TTR("Task status request failed.")));
		_set_tab_status(p_tab_id, error, true);
		_append_message(p_tab_id, TTR("System"), vformat("Task status request failed: %s", error));
		return true;
	}

	_set_tab_status(p_tab_id, TTR("Working..."), false);
	return false;
}

void UltimateAssistantPanel::_finalize_task_poll_terminal_state(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	tab.status_poll_active = false;
	tab.status_poll_request_in_flight = false;
	tab.next_status_poll_msec = 0;
	if (tab.assistant_stream_open && !tab.command_stream_active && !tab.suppress_next_chat_message) {
		_append_stream_delta_for_tab(tab.id, String(), String(), true);
	}
	if (!tab.command_stream_active) {
		_set_tab_loading_state(tab.id, false);
	}
	_set_tab_busy(tab.id, false);
}

bool UltimateAssistantPanel::_poll_task_status_for_tab(int p_tab_id, const String &p_plan_id) {
	if (!backend_adapter || p_plan_id.is_empty()) {
		return false;
	}

	Dictionary response = backend_adapter->get_task_status(p_plan_id);
	_log_request_context("task/status", response);
	return _handle_task_status_response_for_tab(p_tab_id, p_plan_id, response);
}

void UltimateAssistantPanel::_submit_approval_for_tab(int p_tab_id, const String &p_decision) {
	if (!backend_adapter) {
		_set_tab_status(p_tab_id, TTR("Backend adapter unavailable."), true);
		return;
	}

	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];

	if (tab.last_plan_id.is_empty() || tab.pending_action_ids.is_empty()) {
		_set_tab_status(p_tab_id, TTR("No pending approval actions."), true);
		return;
	}
	if (tab.approval_request_in_flight) {
		return;
	}

	_set_tab_busy(p_tab_id, true, TTR("Submitting approval..."));

	Array action_ids;
	for (int i = 0; i < tab.pending_action_ids.size(); i++) {
		action_ids.push_back(tab.pending_action_ids[i]);
	}

	String reviewer = "editor-user";
	Dictionary runtime_config = backend_adapter->get_runtime_config();
	String configured_reviewer = _sanitize_reviewer_id(String(runtime_config.get("actor_id", String())));
	if (!configured_reviewer.is_empty()) {
		reviewer = configured_reviewer;
	}

	Dictionary payload;
	payload["schema_version"] = "v1";
	payload["session_id"] = tab.session_id;
	payload["plan_id"] = tab.last_plan_id;
	payload["decision"] = p_decision;
	payload["approved_action_ids"] = p_decision == "approve" ? action_ids : Array();
	payload["rejected_action_ids"] = p_decision == "reject" ? action_ids : Array();
	payload["reviewer_id"] = reviewer;
	payload["decided_at"] = _now_iso8601_utc();

	tab.approval_request_in_flight = true;
	tab.approval_generation += 1;

	AsyncApprovalJob *job = memnew(AsyncApprovalJob);
	job->tab_id = p_tab_id;
	job->approval_generation = tab.approval_generation;
	job->plan_id = tab.last_plan_id;
	job->decision = p_decision;
	job->runtime_config = backend_adapter->get_runtime_config();
	job->payload = payload;

	_enqueue_async_approval(job);
}

void UltimateAssistantPanel::_execute_commands_for_tab(int p_tab_id, const Array &p_commands) {
	for (int i = 0; i < p_commands.size(); i++) {
		Variant cmd_v = p_commands[i];
		if (cmd_v.get_type() != Variant::DICTIONARY) {
			continue;
		}
		_execute_single_command(p_tab_id, cmd_v);
	}
}

void UltimateAssistantPanel::_execute_single_command(int p_tab_id, const Dictionary &p_command) {
	if (!backend_adapter) {
		return;
	}

	int tab_index = _find_tab_index_by_id(p_tab_id);

	Dictionary trust = backend_adapter->evaluate_command_trust(p_command);
	bool allowed = bool(trust.get("allowed", false));
	bool trusted = bool(trust.get("trusted", false));
	if (!allowed || !trusted) {
		String reason = String(trust.get("reason", "blocked"));
		_append_message(p_tab_id, TTR("System"), vformat("Blocked command execution: %s", reason));
		return;
	}

	String action = String(p_command.get("action", ""));
	if (action == "chat_message") {
		String content = String(p_command.get("content", ""));
		String agent = String(p_command.get("agent", "Assistant"));
		String role = agent.is_empty() ? TTR("Assistant") : agent;
		String normalized_role = role.strip_edges().to_lower();

		if (tab_index >= 0 && tab_index < tabs.size() && normalized_role == "assistant" && !content.strip_edges().is_empty()) {
			ChatTab &tab = tabs.write[tab_index];
			if (!tab.assistant_stream_text.is_empty() && tab.assistant_stream_prefix_len >= 0 && tab.assistant_stream_prefix_len <= tab.transcript.length()) {
				String streamed_text = tab.assistant_stream_text;
				if (content == streamed_text) {
					tab.suppress_next_chat_message = false;
					tab.assistant_stream_text.clear();
					tab.assistant_stream_prefix_len = 0;
					tab.assistant_stream_from_realtime = false;
					return;
				}

				tab.transcript = tab.transcript.substr(0, tab.assistant_stream_prefix_len);
				if (tab.chat_display) {
					tab.chat_display->set_text(tab.transcript);
					tab.chat_display->scroll_to_line(tab.chat_display->get_line_count());
				}
				tab.assistant_stream_open = false;
				tab.assistant_stream_text.clear();
				tab.assistant_stream_prefix_len = 0;
				tab.assistant_stream_from_realtime = false;
				tab.suppress_next_chat_message = false;
				_append_message(p_tab_id, role, content);
				return;
			}
		}
		if (tab_index >= 0 && tab_index < tabs.size() && tabs[tab_index].suppress_next_chat_message) {
			ChatTab &tab = tabs.write[tab_index];
			tab.suppress_next_chat_message = false;

			if (normalized_role == "assistant" && !content.strip_edges().is_empty() && tab.assistant_stream_open) {
				String streamed_text = tab.assistant_stream_text;
				if (content.begins_with(streamed_text)) {
					String remainder = content.substr(streamed_text.length(), content.length() - streamed_text.length());
					_append_stream_delta_for_tab(p_tab_id, role, remainder, true);
					tab.assistant_stream_text.clear();
					tab.assistant_stream_prefix_len = 0;
					tab.assistant_stream_from_realtime = false;
					return;
				}

				if (tab.assistant_stream_prefix_len >= 0 && tab.assistant_stream_prefix_len <= tab.transcript.length()) {
					tab.transcript = tab.transcript.substr(0, tab.assistant_stream_prefix_len);
					if (tab.chat_display) {
						tab.chat_display->set_text(tab.transcript);
						tab.chat_display->scroll_to_line(tab.chat_display->get_line_count());
					}
				}

				tab.assistant_stream_open = false;
				tab.assistant_stream_text.clear();
				tab.assistant_stream_prefix_len = 0;
				tab.assistant_stream_from_realtime = false;
				_append_message(p_tab_id, role, content);
				return;
			}

			return;
		}

		if (normalized_role == "assistant" && !content.strip_edges().is_empty()) {
			_start_command_stream_for_tab(p_tab_id, role, content);
		} else {
			_append_message(p_tab_id, role, content);
		}
		if (tab_index >= 0 && tab_index < tabs.size()) {
			tabs.write[tab_index].assistant_stream_text.clear();
			tabs.write[tab_index].assistant_stream_prefix_len = 0;
			tabs.write[tab_index].assistant_stream_from_realtime = false;
			tabs.write[tab_index].suppress_next_chat_message = false;
		}
		return;
	}

	if (action == "open_docs_query") {
		ScriptEditor *script_editor = ScriptEditor::get_singleton();
		if (!script_editor) {
			_append_message(p_tab_id, TTR("System"), "Failed to open docs query: script editor unavailable.");
			return;
		}

		String query = String(p_command.get("content", String())).strip_edges();
		Vector<String> class_candidates;

		String explicit_class_name = _sanitize_docs_identifier(String(p_command.get("class_name", String())));
		_append_unique_docs_candidate(explicit_class_name, class_candidates);

		Variant candidates_variant = p_command.get("candidates", Variant());
		if (candidates_variant.get_type() == Variant::ARRAY) {
			Array candidates = candidates_variant;
			for (int i = 0; i < candidates.size(); i++) {
				String candidate = _sanitize_docs_identifier(String(candidates[i]));
				_append_unique_docs_candidate(candidate, class_candidates);
			}
		}

		const bool has_explicit_class_candidates = !class_candidates.is_empty();
		if (!has_explicit_class_candidates) {
			_extract_docs_candidates_from_text(query, class_candidates);
		}

		String resolved_class_name;
		for (int i = 0; i < class_candidates.size(); i++) {
			const String &candidate = class_candidates[i];
			if (ClassDB::class_exists(candidate)) {
				resolved_class_name = candidate;
				break;
			}
		}

		if (!resolved_class_name.is_empty()) {
			script_editor->goto_help("class:" + resolved_class_name);
			_append_message(p_tab_id, TTR("System"), vformat("Opened in-editor docs for class: %s", resolved_class_name));
			return;
		}

		String topic = String(p_command.get("topic", String())).strip_edges();
		if (topic.is_empty()) {
			String raw_class_name = String(p_command.get("class_name", String())).strip_edges();
			if (raw_class_name.begins_with("@")) {
				String normalized_class_name = raw_class_name.substr(1, raw_class_name.length() - 1).strip_edges();
				if (!normalized_class_name.is_empty()) {
					topic = "class_name:" + normalized_class_name;
				}
			}
		}
		if (topic.is_empty() && !has_explicit_class_candidates && _is_allowed_help_topic(query)) {
			topic = query;
		}

		if (_is_allowed_help_topic(topic)) {
			script_editor->goto_help(topic);
			_append_message(p_tab_id, TTR("System"), vformat("Opened in-editor docs topic: %s", topic));
			return;
		}

		_append_message(
				p_tab_id,
				TTR("System"),
				"Could not resolve a Godot class/topic from docs query. Ask with a specific class name such as Node, Node3D, CharacterBody2D, or AnimationTree.");
		return;
	}

	if (action == "open_docs_url") {
		String url = String(p_command.get("content", String())).strip_edges();
		if (url.is_empty()) {
			_append_message(p_tab_id, TTR("System"), "Rejected open_docs_url command: content URL is required.");
			return;
		}

		String normalized_url = url.to_lower();
		if (!normalized_url.begins_with("https://") && !normalized_url.begins_with("http://")) {
			_append_message(p_tab_id, TTR("System"), vformat("Rejected open_docs_url '%s'. Only http/https URLs are allowed.", url));
			return;
		}

		Error open_error = OS::get_singleton()->shell_open(url);
		if (open_error != OK) {
			_append_message(p_tab_id, TTR("System"), vformat("Failed to open docs URL '%s': %s", url, VariantUtilityFunctions::error_string(open_error)));
			return;
		}

		_append_message(p_tab_id, TTR("System"), vformat("Opened docs URL: %s", url));
		return;
	}

	if (action == "open_docs_file") {
		String raw_path = String(p_command.get("content", String())).strip_edges();
		if (raw_path.is_empty()) {
			_append_message(p_tab_id, TTR("System"), "Rejected open_docs_file command: content path is required.");
			return;
		}

		String local_path = raw_path;
		if (local_path.is_absolute_path()) {
			local_path = ProjectSettings::get_singleton()->localize_path(local_path);
		}

		if (!_is_supported_command_path(local_path)) {
			_append_message(
					p_tab_id,
					TTR("System"),
					vformat("Rejected open_docs_file '%s'. Only res:// or user:// paths without '..' are allowed.", raw_path));
			return;
		}

		if (!FileAccess::exists(local_path)) {
			_append_message(p_tab_id, TTR("System"), vformat("Docs file does not exist: %s", local_path));
			return;
		}

		EditorNode *editor_node = EditorNode::get_singleton();
		if (!editor_node) {
			_append_message(p_tab_id, TTR("System"), "Failed to open docs file: editor node unavailable.");
			return;
		}

		Error open_error = editor_node->load_scene_or_resource(local_path, false, true);
		if (open_error != OK) {
			String absolute_path = ProjectSettings::get_singleton()->globalize_path(local_path);
			Error shell_error = OS::get_singleton()->shell_open(absolute_path);
			if (shell_error != OK) {
				_append_message(
						p_tab_id,
						TTR("System"),
						vformat("Failed to open docs file '%s': %s", local_path, VariantUtilityFunctions::error_string(open_error)));
				return;
			}

			_append_message(p_tab_id, TTR("System"), vformat("Opened local docs file in external viewer: %s", local_path));
			return;
		}

		_append_message(p_tab_id, TTR("System"), vformat("Opened local docs file in editor: %s", local_path));
		return;
	}

	if (action == "create_file") {
		String path = String(p_command.get("path", String())).strip_edges();
		if (!_is_supported_command_path(path)) {
			_append_message(p_tab_id, TTR("System"), vformat("Rejected create_file path '%s'. Only res:// or user:// paths without '..' are allowed.", path));
			return;
		}

		String raw_base64 = String(p_command.get("data_base64", String())).strip_edges();
		String content = String(p_command.get("content", String()));
		if (raw_base64.is_empty() && content.is_empty()) {
			_append_message(p_tab_id, TTR("System"), "Rejected create_file command: either content or data_base64 is required.");
			return;
		}

		String base_dir = path.get_base_dir();
		if (!base_dir.is_empty()) {
			String absolute_dir = ProjectSettings::get_singleton()->globalize_path(base_dir);
			Error mkdir_err = DirAccess::make_dir_recursive_absolute(absolute_dir);
			if (mkdir_err != OK) {
				_append_message(p_tab_id, TTR("System"), vformat("Failed to create directory for '%s': %s", path, VariantUtilityFunctions::error_string(mkdir_err)));
				return;
			}
		}

		Vector<uint8_t> file_bytes;
		if (!raw_base64.is_empty() && !_decode_base64_bytes(raw_base64, file_bytes)) {
			_append_message(p_tab_id, TTR("System"), vformat("Rejected create_file for '%s': invalid data_base64 payload.", path));
			return;
		}

		Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
		if (file.is_null()) {
			Error open_error = FileAccess::get_open_error();
			_append_message(p_tab_id, TTR("System"), vformat("Failed to open '%s' for writing: %s", path, VariantUtilityFunctions::error_string(open_error)));
			return;
		}

		if (!raw_base64.is_empty()) {
			if (!file_bytes.is_empty()) {
				file->store_buffer(file_bytes.ptr(), file_bytes.size());
			}
		} else {
			file->store_string(content);
		}
		file.unref();

		EditorFileSystem *filesystem = EditorFileSystem::get_singleton();
		if (filesystem) {
			filesystem->scan_changes();
		}

		_append_message(p_tab_id, TTR("System"), vformat("Created file: %s", path));
		return;
	}

	if (action == "modify_text") {
		String file_path = String(p_command.get("file", String())).strip_edges();
		if (!_is_supported_command_path(file_path)) {
			_append_message(p_tab_id, TTR("System"), vformat("Rejected modify_text file '%s'. Only res:// or user:// paths without '..' are allowed.", file_path));
			return;
		}

		String search_text = String(p_command.get("search", String()));
		if (search_text.is_empty()) {
			_append_message(p_tab_id, TTR("System"), "Rejected modify_text command: search text is required.");
			return;
		}

		String replace_text = String(p_command.get("replace", p_command.get("content", String())));
		if (replace_text.is_empty()) {
			_append_message(p_tab_id, TTR("System"), "Rejected modify_text command: replace or content is required.");
			return;
		}

		Ref<FileAccess> read_file = FileAccess::open(file_path, FileAccess::READ);
		if (read_file.is_null()) {
			Error open_error = FileAccess::get_open_error();
			_append_message(p_tab_id, TTR("System"), vformat("Failed to open '%s' for reading: %s", file_path, VariantUtilityFunctions::error_string(open_error)));
			return;
		}

		String source_text = read_file->get_as_text();
		read_file.unref();

		int found_at = source_text.find(search_text);
		if (found_at < 0) {
			_append_message(p_tab_id, TTR("System"), vformat("modify_text could not find target text in '%s'.", file_path));
			return;
		}

		bool insert_after = bool(p_command.get("insert_after", false));
		const int search_end = found_at + search_text.length();
		String updated_text;
		if (insert_after) {
			updated_text = source_text.substr(0, search_end) + replace_text + source_text.substr(search_end, source_text.length() - search_end);
		} else {
			updated_text = source_text.substr(0, found_at) + replace_text + source_text.substr(search_end, source_text.length() - search_end);
		}

		Ref<FileAccess> write_file = FileAccess::open(file_path, FileAccess::WRITE);
		if (write_file.is_null()) {
			Error open_error = FileAccess::get_open_error();
			_append_message(p_tab_id, TTR("System"), vformat("Failed to open '%s' for writing: %s", file_path, VariantUtilityFunctions::error_string(open_error)));
			return;
		}
		write_file->store_string(updated_text);
		write_file.unref();

		EditorFileSystem *filesystem = EditorFileSystem::get_singleton();
		if (filesystem) {
			filesystem->scan_changes();
		}

		_append_message(p_tab_id, TTR("System"), vformat("Applied modify_text to: %s", file_path));
		return;
	}

	if (action == "create_node") {
		String parent_path = String(p_command.get("parent", String())).strip_edges();
		String node_type = String(p_command.get("type", String())).strip_edges();
		String node_name = String(p_command.get("name", String())).strip_edges();

		if (node_type.is_empty() || node_name.is_empty()) {
			_append_message(p_tab_id, TTR("System"), "Rejected create_node command: type and name are required.");
			return;
		}

		SceneTree *tree = get_tree();
		Node *edited_scene_root = tree ? tree->get_edited_scene_root() : nullptr;
		if (!edited_scene_root) {
			_append_message(p_tab_id, TTR("System"), "create_node requires an active edited scene root.");
			return;
		}

		Node *parent_node = _resolve_command_parent_node(edited_scene_root, parent_path);
		if (!parent_node) {
			_append_message(p_tab_id, TTR("System"), vformat("create_node could not resolve parent '%s'.", parent_path));
			return;
		}

		Object *instance = ClassDB::instantiate(StringName(node_type));
		if (!instance) {
			_append_message(p_tab_id, TTR("System"), vformat("create_node failed: unknown type '%s'.", node_type));
			return;
		}

		Node *new_node = Object::cast_to<Node>(instance);
		if (!new_node) {
			memdelete(instance);
			_append_message(p_tab_id, TTR("System"), vformat("create_node failed: type '%s' is not a Node.", node_type));
			return;
		}

		new_node->set_name(node_name);
		parent_node->add_child(new_node);

		Node *owner = parent_node == edited_scene_root ? edited_scene_root : parent_node->get_owner();
		if (!owner) {
			owner = edited_scene_root;
		}
		new_node->set_owner(owner);

		Variant properties_v = p_command.get("properties", Variant());
		if (properties_v.get_type() == Variant::DICTIONARY) {
			Dictionary properties = properties_v;
			Array keys = properties.keys();
			for (int i = 0; i < keys.size(); i++) {
				String key = String(keys[i]).strip_edges();
				if (key.is_empty()) {
					continue;
				}
				new_node->set(StringName(key), properties[keys[i]]);
			}
		}

		_append_message(p_tab_id, TTR("System"), vformat("Created node '%s' (%s) under '%s'.", node_name, node_type, parent_node->get_name()));
		return;
	}

	_append_message(p_tab_id, TTR("System"), vformat("Allowed command '%s' is not mapped in MVP executor.", action));
}

void UltimateAssistantPanel::_log_request_context(const String &p_operation, const Dictionary &p_response) const {
	String request_id = String(p_response.get("request_id", ""));
	String correlation_id = String(p_response.get("correlation_id", ""));
	int status = int(p_response.get("status_code", 0));
	print_line(vformat("[UltimateAssistantPanel] op=%s status=%d request_id=%s correlation_id=%s", p_operation, status, request_id, correlation_id));
}

void UltimateAssistantPanel::_on_approve_pressed(int p_tab_id) {
	_submit_approval_for_tab(p_tab_id, "approve");
}

void UltimateAssistantPanel::_on_reject_pressed(int p_tab_id) {
	_submit_approval_for_tab(p_tab_id, "reject");
}

void UltimateAssistantPanel::_on_resync_pressed(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];

	if (tab.has_conflict) {
		_set_tab_busy(p_tab_id, true, TTR("Resyncing session..."));
		if (_start_session_for_tab(p_tab_id, true)) {
			_clear_conflict_state(p_tab_id);
			_set_tab_status(p_tab_id, TTR("Session resynced."), false);
			_append_message(p_tab_id, TTR("System"), TTR("Session resynced successfully."));
		}
		_set_tab_busy(p_tab_id, false);
		return;
	}

	if (!tab.last_plan_id.is_empty()) {
		if (tab.status_poll_request_in_flight) {
			return;
		}

		tab.status_poll_request_in_flight = true;
		tab.status_poll_generation += 1;
		_set_tab_busy(p_tab_id, true, TTR("Refreshing gateway task status..."));

		AsyncTaskStatusPollJob *status_job = memnew(AsyncTaskStatusPollJob);
		status_job->tab_id = p_tab_id;
		status_job->poll_generation = tab.status_poll_generation;
		status_job->plan_id = tab.last_plan_id;
		status_job->one_shot_refresh = true;
		status_job->runtime_config = backend_adapter ? backend_adapter->get_runtime_config() : Dictionary();
		_enqueue_async_task_status_poll(status_job);
		return;
	}

	_set_tab_status(p_tab_id, TTR("No conflict or gateway task to resync."), false);
}

void UltimateAssistantPanel::_on_settings_pressed() {
	settings_dialog->set_models(available_models);
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	bool thinking_enabled = false;
	if (project_settings && project_settings->has_setting("phoenix/assistant/show_thinking_stream")) {
		thinking_enabled = bool(project_settings->get("phoenix/assistant/show_thinking_stream"));
	}
	settings_dialog->set_thinking_stream_enabled(thinking_enabled);
	if (backend_adapter) {
		settings_dialog->set_runtime_config(backend_adapter->get_runtime_config());
	}
	settings_dialog->popup_centered();
}

void UltimateAssistantPanel::_on_settings_confirmed() {
	PackedStringArray selected = settings_dialog->get_selected_models();
	if (!selected.is_empty()) {
		available_models = selected;
		_refresh_model_selectors();
	}
	if (backend_adapter) {
		backend_adapter->apply_runtime_config(settings_dialog->get_runtime_config());
	}
	_refresh_all_tool_selectors();

	bool thinking_enabled = settings_dialog->is_thinking_stream_enabled();
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (project_settings) {
		project_settings->set("phoenix/assistant/show_thinking_stream", thinking_enabled);
		project_settings->save();
	}

	if (!thinking_enabled) {
		for (int i = 0; i < tabs.size(); i++) {
			if (tabs[i].thinking_stream_open) {
				_append_thinking_delta_for_tab(tabs[i].id, String(), true);
			}
		}
	}

	_broadcast_shared_state();
}

void UltimateAssistantPanel::_on_tab_setting_changed(int p_selected_index, int p_tab_id) {
	(void)p_selected_index;
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index >= 0 && tab_index < tabs.size()) {
		_refresh_tool_selector_for_tab(tabs.write[tab_index]);
	}
	_refresh_hub();
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_on_tool_selection_changed(int p_index, bool p_selected, int p_tab_id) {
	(void)p_index;
	(void)p_selected;
	(void)p_tab_id;
	_refresh_hub();
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_on_context_add_pressed(int p_tab_id) {
	_open_context_dialog(p_tab_id);
}

void UltimateAssistantPanel::_on_context_remove_pressed(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	ItemList *list = tabs[tab_index].context_list;
	if (!list) {
		return;
	}
	PackedInt32Array selected = list->get_selected_items();
	for (int i = selected.size() - 1; i >= 0; i--) {
		list->remove_item(selected[i]);
	}
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_on_context_toggle_toggled(bool p_pressed, int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	tab.context_collapsed = !p_pressed;
	if (tab.context_list) {
		tab.context_list->set_visible(p_pressed);
	}
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_on_context_select_add_pressed() {
	if (!context_file_list || !context_selected_list) {
		return;
	}
	PackedInt32Array indices = context_file_list->get_selected_items();
	for (int i = 0; i < indices.size(); i++) {
		int file_idx = indices[i];
		String label = context_file_list->get_item_text(file_idx);
		Variant item_meta = context_file_list->get_item_metadata(file_idx);
		Variant store_meta = item_meta.get_type() == Variant::NIL ? Variant(label) : item_meta;
		String display_text = item_meta.get_type() == Variant::STRING ? String(item_meta) : label;

		bool exists = false;
		int count = context_selected_list->get_item_count();
		for (int j = 0; j < count; j++) {
			Variant existing_meta = context_selected_list->get_item_metadata(j);
			if (existing_meta.get_type() != Variant::NIL && store_meta.get_type() != Variant::NIL) {
				if (existing_meta == store_meta) {
					exists = true;
					break;
				}
				continue;
			}
			if (context_selected_list->get_item_text(j) == display_text) {
				exists = true;
				break;
			}
		}
		if (!exists) {
			int idx = context_selected_list->add_item(display_text);
			context_selected_list->set_item_metadata(idx, store_meta);
		}
	}
}

void UltimateAssistantPanel::_on_context_select_remove_pressed() {
	if (!context_selected_list) {
		return;
	}
	PackedInt32Array indices = context_selected_list->get_selected_items();
	for (int i = indices.size() - 1; i >= 0; i--) {
		context_selected_list->remove_item(indices[i]);
	}
}

void UltimateAssistantPanel::_on_context_note_add_pressed() {
	if (!context_note_input || !context_selected_list) {
		return;
	}
	String value = context_note_input->get_text().strip_edges();
	if (value.is_empty()) {
		return;
	}
	int idx = context_selected_list->add_item(value);
	context_selected_list->set_item_metadata(idx, value);
	context_note_input->clear();
}

void UltimateAssistantPanel::_on_context_search_changed(const String &p_text) {
	_populate_context_file_list(p_text);
}

void UltimateAssistantPanel::_on_context_dialog_confirmed() {
	int tab_index = _find_tab_index_by_id(context_dialog_tab_id);
	if (tab_index < 0) {
		return;
	}
	ItemList *list = tabs[tab_index].context_list;
	if (!list || !context_selected_list) {
		return;
	}
	list->clear();
	int count = context_selected_list->get_item_count();
	for (int i = 0; i < count; i++) {
		String text = context_selected_list->get_item_text(i);
		Variant meta = context_selected_list->get_item_metadata(i);
		int idx = list->add_item(text);
		if (meta.get_type() != Variant::NIL) {
			list->set_item_metadata(idx, meta);
		}
	}
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_on_previous_reopen_pressed() {
	if (!hub_previous_list) {
		return;
	}
	PackedInt32Array selected = hub_previous_list->get_selected_items();
	if (selected.is_empty()) {
		return;
	}
	Variant meta = hub_previous_list->get_item_metadata(selected[0]);
	if (meta.get_type() != Variant::INT) {
		return;
	}
	int idx = meta;
	_restore_archived_session(idx);
}

void UltimateAssistantPanel::_on_previous_session_activated(int p_index) {
	if (!hub_previous_list) {
		return;
	}
	Variant meta = hub_previous_list->get_item_metadata(p_index);
	if (meta.get_type() != Variant::INT) {
		return;
	}
	int idx = meta;
	_restore_archived_session(idx);
}

void UltimateAssistantPanel::_on_session_name_changed(const String &p_text, int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	tab.display_name = p_text.strip_edges();
	int container_index = _find_tab_container_index(tab.root);
	if (container_index >= 0) {
		tab_container->set_tab_title(container_index, _get_tab_label(tab));
	}
	_refresh_hub();
	_broadcast_shared_state();
}

void UltimateAssistantPanel::_open_context_dialog(int p_tab_id) {
	context_dialog_tab_id = p_tab_id;
	if (context_search_input) {
		context_search_input->clear();
	}
	if (context_note_input) {
		context_note_input->clear();
	}
	_sync_context_selected_list(p_tab_id);
	_populate_context_file_list(String());
	context_dialog->popup_centered();
}

void UltimateAssistantPanel::_populate_context_file_list(const String &p_filter) {
	if (!context_file_list) {
		return;
	}
	context_file_list->clear();
	String filter = p_filter.strip_edges();
	_populate_pixelpen_context_items(filter);

	Vector<String> files;
	EditorFileSystem *fs = EditorFileSystem::get_singleton();
	if (fs) {
		EditorFileSystemDirectory *root_dir = fs->get_filesystem();
		if (root_dir) {
			_collect_project_files(root_dir, files);
		}
	}
	for (int i = 0; i < files.size(); i++) {
		if (!filter.is_empty() && files[i].findn(filter) == -1) {
			continue;
		}
		String type = EditorFileSystem::get_singleton() ? EditorFileSystem::get_singleton()->get_file_type(files[i]) : String();
		String label = type.is_empty() ? files[i] : vformat("%s  (%s)", files[i], type);
		Ref<Texture2D> icon = _get_file_icon_for_path(files[i]);
		int idx = context_file_list->add_item(label, icon);
		context_file_list->set_item_metadata(idx, files[i]);
	}
}

void UltimateAssistantPanel::_collect_project_files(EditorFileSystemDirectory *p_dir, Vector<String> &r_files) const {
	if (!p_dir) {
		return;
	}
	int file_count = p_dir->get_file_count();
	for (int i = 0; i < file_count; i++) {
		r_files.push_back(p_dir->get_file_path(i));
	}
	int dir_count = p_dir->get_subdir_count();
	for (int i = 0; i < dir_count; i++) {
		_collect_project_files(p_dir->get_subdir(i), r_files);
	}
}

void UltimateAssistantPanel::_sync_context_selected_list(int p_tab_id) {
	if (!context_selected_list) {
		return;
	}
	context_selected_list->clear();
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	ItemList *list = tabs[tab_index].context_list;
	if (!list) {
		return;
	}
	int count = list->get_item_count();
	for (int i = 0; i < count; i++) {
		String text = list->get_item_text(i);
		Variant meta = list->get_item_metadata(i);
		int idx = context_selected_list->add_item(text);
		if (meta.get_type() != Variant::NIL) {
			context_selected_list->set_item_metadata(idx, meta);
		}
	}
}

Ref<Texture2D> UltimateAssistantPanel::_get_file_icon_for_path(const String &p_path) const {
	EditorFileSystem *fs = EditorFileSystem::get_singleton();
	if (!fs) {
		return Ref<Texture2D>();
	}
	String dir_path = p_path.get_base_dir();
	EditorFileSystemDirectory *dir = fs->get_filesystem_path(dir_path);
	if (!dir) {
		return Ref<Texture2D>();
	}
	int idx = dir->find_file_index(p_path.get_file());
	if (idx < 0) {
		return Ref<Texture2D>();
	}
	String icon_path = dir->get_file_icon_path(idx);
	if (icon_path.is_empty()) {
		return Ref<Texture2D>();
	}
	Ref<Resource> res = ResourceLoader::load(icon_path);
	Ref<Texture2D> tex = res;
	return tex;
}

void UltimateAssistantPanel::_refresh_hub() {
	if (!hub_agent_list) {
		return;
	}
	hub_agent_list->clear();
	if (hub_previous_list) {
		hub_previous_list->clear();
	}
	if (hub_pending_list) {
		hub_pending_list->clear();
	}
	if (hub_questions_list) {
		hub_questions_list->clear();
	}
	for (int i = 0; i < tabs.size(); i++) {
		const ChatTab &tab = tabs[i];
		String model = TTR("Unknown");
		if (tab.model_selector && tab.model_selector->get_selected() >= 0) {
			model = tab.model_selector->get_item_text(tab.model_selector->get_selected());
		}
		String agent_mode = TTR("Local");
		if (tab.agent_mode_selector && tab.agent_mode_selector->get_selected() >= 0) {
			agent_mode = tab.agent_mode_selector->get_item_text(tab.agent_mode_selector->get_selected());
		}
		String status = tab.is_active ? TTR("Busy") : TTR("Idle");
		if (tab.has_conflict) {
			status = TTR("Conflict");
		} else if (!tab.pending_action_ids.is_empty()) {
			status = TTR("Awaiting approval");
		} else if (!tab.last_plan_id.is_empty()) {
			status = TTR("Gateway queued");
		}
		String entry = vformat("%s  |  %s  |  %s  |  %s", _get_tab_label(tab), model, agent_mode, status);
		hub_agent_list->add_item(entry);

		if (hub_pending_list) {
			if (tab.has_conflict) {
				hub_pending_list->add_item(vformat("%s  |  Conflict 409 detected (resync required)", _get_tab_label(tab)));
			}
			if (!tab.last_plan_id.is_empty() && tab.pending_action_ids.is_empty() && !tab.has_conflict) {
				hub_pending_list->add_item(vformat("%s  |  Gateway task pending: %s", _get_tab_label(tab), tab.last_plan_id));
			}
			for (int j = 0; j < tab.pending_action_ids.size(); j++) {
				hub_pending_list->add_item(vformat("%s  |  Pending approval: %s", _get_tab_label(tab), tab.pending_action_ids[j]));
			}
		}
		if (hub_questions_list) {
			if (tab.has_conflict) {
				hub_questions_list->add_item(vformat("%s  |  Action needed: click Resync before retrying requests.", _get_tab_label(tab)));
			} else if (!tab.last_plan_id.is_empty() && tab.pending_action_ids.is_empty()) {
				hub_questions_list->add_item(vformat("%s  |  Action needed: click Resync to refresh gateway task status.", _get_tab_label(tab)));
			}
		}
	}
	if (hub_previous_list) {
		for (int i = 0; i < archived_sessions.size(); i++) {
			const ArchivedSession &archived = archived_sessions[i];
			String entry = vformat("%s  |  %s  |  %s", archived.display_title, archived.model, archived.agent_mode);
			int item_idx = hub_previous_list->add_item(entry);
			hub_previous_list->set_item_metadata(item_idx, i);
		}
	}
}

void UltimateAssistantPanel::_archive_tab(const ChatTab &p_tab) {
	ArchivedSession archived;
	archived.custom_name = p_tab.display_name.strip_edges();
	archived.display_title = _get_tab_label(p_tab);
	archived.model = TTR("Unknown");
	if (p_tab.model_selector && p_tab.model_selector->get_selected() >= 0) {
		archived.model = p_tab.model_selector->get_item_text(p_tab.model_selector->get_selected());
	}
	archived.agent_mode = TTR("Local");
	if (p_tab.agent_mode_selector && p_tab.agent_mode_selector->get_selected() >= 0) {
		archived.agent_mode = p_tab.agent_mode_selector->get_item_text(p_tab.agent_mode_selector->get_selected());
	}
	archived.transcript = p_tab.transcript;
	if (p_tab.context_list) {
		int count = p_tab.context_list->get_item_count();
		for (int i = 0; i < count; i++) {
			archived.context_items.push_back(p_tab.context_list->get_item_text(i));
			archived.context_metadata.push_back(p_tab.context_list->get_item_metadata(i));
		}
	}
	archived_sessions.push_back(archived);
}

void UltimateAssistantPanel::_restore_archived_session(int p_index) {
	if (p_index < 0 || p_index >= archived_sessions.size()) {
		return;
	}
	ArchivedSession archived = archived_sessions[p_index];
	archived_sessions.remove_at(p_index);

	int previous_count = tabs.size();
	_add_chat_tab();
	if (previous_count < 0 || previous_count >= tabs.size()) {
		_refresh_hub();
		return;
	}
	ChatTab &tab = tabs.write[previous_count];
	if (!archived.custom_name.is_empty()) {
		tab.display_name = archived.custom_name;
		if (tab.session_name_input) {
			tab.session_name_input->set_text(archived.custom_name);
		}
	}
	if (tab.chat_display) {
		if (!archived.transcript.is_empty()) {
			tab.chat_display->set_text(archived.transcript);
			tab.transcript = archived.transcript;
		}
	}
	if (tab.context_list && archived.context_items.size() > 0) {
		for (int i = 0; i < archived.context_items.size(); i++) {
			int idx = tab.context_list->add_item(archived.context_items[i]);
			Variant meta = archived.context_metadata.size() > i ? archived.context_metadata[i] : Variant();
			if (meta.get_type() != Variant::NIL) {
				tab.context_list->set_item_metadata(idx, meta);
			}
		}
	}
	int container_index = _find_tab_container_index(tab.root);
	if (container_index >= 0) {
		tab_container->set_tab_title(container_index, _get_tab_label(tab));
	}
	_refresh_hub();
	_broadcast_shared_state();
}

String UltimateAssistantPanel::_get_tab_label(const ChatTab &p_tab) const {
	String name = p_tab.display_name.strip_edges();
	if (name.is_empty()) {
		return vformat("Agent %d", p_tab.id);
	}
	return name;
}
