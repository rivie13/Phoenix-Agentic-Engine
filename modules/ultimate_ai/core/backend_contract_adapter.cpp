/**************************************************************************/
/*  backend_contract_adapter.cpp                                          */
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

#include "backend_contract_adapter.h"

#include "core/crypto/crypto.h"
#include "core/io/json.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/print_string.h"

namespace {
const char *const HEADER_REQUEST_ID = "x-request-id";
const char *const HEADER_CORRELATION_ID = "x-correlation-id";
const char *const HEADER_AUTHORIZATION = "Authorization";

inline bool _is_success_status(int p_status_code) {
	return p_status_code >= 200 && p_status_code < 300;
}

inline bool _is_retryable_status(int p_status_code) {
	return p_status_code == HTTPClient::RESPONSE_TOO_MANY_REQUESTS ||
			p_status_code == HTTPClient::RESPONSE_BAD_GATEWAY ||
			p_status_code == HTTPClient::RESPONSE_SERVICE_UNAVAILABLE ||
			p_status_code == HTTPClient::RESPONSE_GATEWAY_TIMEOUT;
}

inline String _trim_header_value(const String &p_line) {
	int sep = p_line.find(":");
	if (sep < 0) {
		return String();
	}
	return p_line.substr(sep + 1, p_line.length()).strip_edges();
}
} //namespace

void UltimateAIBackendContractAdapter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("apply_runtime_config", "config"), &UltimateAIBackendContractAdapter::apply_runtime_config);
	ClassDB::bind_method(D_METHOD("get_runtime_config"), &UltimateAIBackendContractAdapter::get_runtime_config);
	ClassDB::bind_method(D_METHOD("start_session", "payload"), &UltimateAIBackendContractAdapter::start_session);
	ClassDB::bind_method(D_METHOD("request_task", "payload"), &UltimateAIBackendContractAdapter::request_task);
	ClassDB::bind_method(D_METHOD("get_task_status", "plan_id"), &UltimateAIBackendContractAdapter::get_task_status);
	ClassDB::bind_method(D_METHOD("submit_approval", "plan_id", "payload"), &UltimateAIBackendContractAdapter::submit_approval);
	ClassDB::bind_method(D_METHOD("auth_handshake", "payload"), &UltimateAIBackendContractAdapter::auth_handshake);
	ClassDB::bind_method(D_METHOD("list_tools"), &UltimateAIBackendContractAdapter::list_tools);
	ClassDB::bind_method(D_METHOD("invoke_tool", "payload"), &UltimateAIBackendContractAdapter::invoke_tool);
	ClassDB::bind_method(D_METHOD("evaluate_command_trust", "command"), &UltimateAIBackendContractAdapter::evaluate_command_trust);
	ClassDB::bind_method(D_METHOD("get_command_allowlist"), &UltimateAIBackendContractAdapter::get_command_allowlist);
	ClassDB::bind_method(D_METHOD("set_command_allowlist", "allowlist"), &UltimateAIBackendContractAdapter::set_command_allowlist);
}

UltimateAIBackendContractAdapter::UltimateAIBackendContractAdapter() {
	command_allowlist.push_back("chat_message");
}

String UltimateAIBackendContractAdapter::_now_iso8601_utc() const {
	return Time::get_singleton()->get_datetime_string_from_system(true, false) + "Z";
}

String UltimateAIBackendContractAdapter::_generate_request_id(const String &p_prefix) const {
	uint64_t now = OS::get_singleton()->get_ticks_usec();
	uint64_t unix_time = (uint64_t)Time::get_singleton()->get_unix_time_from_system();
	return vformat("%s-%llu-%llu", p_prefix, unix_time, now);
}

String UltimateAIBackendContractAdapter::_resolve_auth_token() const {
	String token = runtime_config.token.strip_edges();
	if (!token.is_empty()) {
		return token;
	}

	String hook = runtime_config.token_hook.strip_edges();
	if (hook.is_empty()) {
		return String();
	}

	if (hook.begins_with("env:")) {
		String env_var = hook.substr(4, hook.length()).strip_edges();
		if (env_var.is_empty()) {
			return String();
		}
		if (!OS::get_singleton()->has_environment(env_var)) {
			return String();
		}
		return OS::get_singleton()->get_environment(env_var).strip_edges();
	}

	return hook;
}

String UltimateAIBackendContractAdapter::_extract_header_value(const List<String> &p_headers, const String &p_key) const {
	String key_lower = p_key.to_lower();
	for (const List<String>::Element *E = p_headers.front(); E; E = E->next()) {
		String line = E->get();
		int sep = line.find(":");
		if (sep < 0) {
			continue;
		}
		String current_key = line.substr(0, sep).strip_edges().to_lower();
		if (current_key == key_lower) {
			return _trim_header_value(line);
		}
	}
	return String();
}

String UltimateAIBackendContractAdapter::_extract_error_message_from_body(const Variant &p_body) const {
	if (p_body.get_type() == Variant::DICTIONARY) {
		Dictionary dict = p_body;
		if (dict.has("detail")) {
			return String(dict["detail"]);
		}
		if (dict.has("error")) {
			return String(dict["error"]);
		}
		if (dict.has("message")) {
			return String(dict["message"]);
		}
	}
	if (p_body.get_type() == Variant::STRING) {
		return String(p_body);
	}
	return String();
}

bool UltimateAIBackendContractAdapter::_is_retryable_failure(const Dictionary &p_response) const {
	if (!p_response.has("ok") || bool(p_response["ok"])) {
		return false;
	}

	if (p_response.has("transport_error") && bool(p_response["transport_error"])) {
		return true;
	}

	if (!p_response.has("status_code")) {
		return false;
	}

	int status_code = int(p_response["status_code"]);
	return _is_retryable_status(status_code);
}

Dictionary UltimateAIBackendContractAdapter::_request_once(HTTPClient::Method p_method, const String &p_path, const Variant &p_body) const {
	Dictionary response;
	response["ok"] = false;
	response["status_code"] = 0;
	response["error"] = "Unknown request error.";
	response["body"] = Variant();
	response["transport_error"] = false;

	String request_id = _generate_request_id("req");
	String correlation_id = _generate_request_id("corr");
	response["request_id"] = request_id;
	response["correlation_id"] = correlation_id;

	String base_url = runtime_config.base_url.strip_edges();
	if (base_url.is_empty()) {
		response["transport_error"] = true;
		response["error"] = "Runtime config missing base_url.";
		return response;
	}

	if (base_url.ends_with("/")) {
		base_url = base_url.substr(0, base_url.length() - 1);
	}
	String full_url = base_url + p_path;

	String scheme;
	String host;
	String url_path;
	String fragment;
	int port = 0;
	Error parse_err = full_url.parse_url(scheme, host, port, url_path, fragment);
	if (parse_err != OK || host.is_empty()) {
		response["transport_error"] = true;
		response["error"] = vformat("Invalid backend URL: %s", full_url);
		return response;
	}

	bool use_tls = scheme == "https://";
	if (port == 0) {
		port = use_tls ? 443 : 80;
	}
	if (url_path.is_empty()) {
		url_path = "/";
	}

	Ref<HTTPClient> client = Ref<HTTPClient>(HTTPClient::create());

	Error connect_err = client->connect_to_host(host, port, use_tls ? TLSOptions::client() : Ref<TLSOptions>());
	if (connect_err != OK) {
		response["transport_error"] = true;
		response["error"] = vformat("Unable to connect to backend host %s:%d (error %d).", host, port, connect_err);
		return response;
	}

	uint64_t started = OS::get_singleton()->get_ticks_msec();
	while (client->get_status() == HTTPClient::STATUS_RESOLVING || client->get_status() == HTTPClient::STATUS_CONNECTING) {
		if (OS::get_singleton()->get_ticks_msec() - started > (uint64_t)runtime_config.timeout_ms) {
			response["transport_error"] = true;
			response["error"] = "Connection timeout while reaching backend.";
			return response;
		}
		Error poll_err = client->poll();
		if (poll_err != OK && poll_err != ERR_BUSY) {
			response["transport_error"] = true;
			response["error"] = vformat("Backend poll failed during connect (error %d).", poll_err);
			return response;
		}
		OS::get_singleton()->delay_usec(1000);
	}

	if (client->get_status() != HTTPClient::STATUS_CONNECTED) {
		response["transport_error"] = true;
		response["error"] = vformat("Backend connection failed with status %d.", client->get_status());
		return response;
	}

	String payload_text;
	if (p_method != HTTPClient::METHOD_GET && p_method != HTTPClient::METHOD_HEAD) {
		if (p_body.get_type() == Variant::DICTIONARY || p_body.get_type() == Variant::ARRAY) {
			payload_text = JSON::stringify(p_body, "", true, true);
		} else if (p_body.get_type() == Variant::STRING) {
			payload_text = String(p_body);
		}
	}

	Vector<String> headers;
	headers.push_back(String("Accept: application/json"));
	headers.push_back(String("Content-Type: application/json"));
	headers.push_back(vformat("%s: %s", HEADER_REQUEST_ID, request_id));
	headers.push_back(vformat("%s: %s", HEADER_CORRELATION_ID, correlation_id));
	headers.push_back(vformat("x-phoenix-auth-mode: %s", runtime_config.auth_mode));
	if (!runtime_config.tier.is_empty()) {
		headers.push_back(vformat("x-phoenix-tier: %s", runtime_config.tier));
	}
	if (!runtime_config.actor_id.is_empty()) {
		headers.push_back(vformat("x-phoenix-actor-id: %s", runtime_config.actor_id));
	}

	String auth_token = _resolve_auth_token();
	if (!auth_token.is_empty()) {
		headers.push_back(vformat("%s: Bearer %s", HEADER_AUTHORIZATION, auth_token));
	}

	PackedByteArray payload_bytes = payload_text.to_utf8_buffer();
	Error request_err = client->request(p_method, url_path, headers, payload_bytes.ptr(), payload_bytes.size());
	if (request_err != OK) {
		response["transport_error"] = true;
		response["error"] = vformat("Backend request could not be sent (error %d).", request_err);
		return response;
	}

	PackedByteArray body_bytes;
	List<String> response_headers;
	started = OS::get_singleton()->get_ticks_msec();
	while (true) {
		if (OS::get_singleton()->get_ticks_msec() - started > (uint64_t)runtime_config.timeout_ms) {
			response["transport_error"] = true;
			response["error"] = "Request timed out while reading backend response.";
			return response;
		}

		HTTPClient::Status status = client->get_status();
		if (status == HTTPClient::STATUS_REQUESTING) {
			Error poll_err = client->poll();
			if (poll_err != OK && poll_err != ERR_BUSY) {
				response["transport_error"] = true;
				response["error"] = vformat("Backend poll failed while requesting (error %d).", poll_err);
				return response;
			}
			OS::get_singleton()->delay_usec(1000);
			continue;
		}

		if (status == HTTPClient::STATUS_BODY) {
			Error poll_err = client->poll();
			if (poll_err != OK && poll_err != ERR_BUSY) {
				response["transport_error"] = true;
				response["error"] = vformat("Backend poll failed while reading body (error %d).", poll_err);
				return response;
			}
			PackedByteArray chunk = client->read_response_body_chunk();
			if (!chunk.is_empty()) {
				body_bytes.append_array(chunk);
			}
			OS::get_singleton()->delay_usec(1000);
			continue;
		}

		if (status == HTTPClient::STATUS_CONNECTED) {
			break;
		}

		if (status == HTTPClient::STATUS_CONNECTION_ERROR || status == HTTPClient::STATUS_TLS_HANDSHAKE_ERROR || status == HTTPClient::STATUS_CANT_CONNECT || status == HTTPClient::STATUS_CANT_RESOLVE) {
			response["transport_error"] = true;
			response["error"] = vformat("Backend request failed with client status %d.", status);
			return response;
		}

		Error poll_err = client->poll();
		if (poll_err != OK && poll_err != ERR_BUSY) {
			response["transport_error"] = true;
			response["error"] = vformat("Backend poll failed with error %d.", poll_err);
			return response;
		}
		OS::get_singleton()->delay_usec(1000);
	}

	if (client->has_response()) {
		response["status_code"] = client->get_response_code();
		client->get_response_headers(&response_headers);
	}

	String header_correlation = _extract_header_value(response_headers, HEADER_CORRELATION_ID);
	if (!header_correlation.is_empty()) {
		response["correlation_id"] = header_correlation;
	}

	String body_text;
	if (!body_bytes.is_empty()) {
		body_text = String::utf8((const char *)body_bytes.ptr(), body_bytes.size());
	}

	Variant parsed_body;
	if (!body_text.is_empty()) {
		parsed_body = JSON::parse_string(body_text);
		if (parsed_body.get_type() == Variant::NIL && body_text.strip_edges() != "null") {
			parsed_body = body_text;
		}
	}
	response["body"] = parsed_body;

	int status_code = int(response["status_code"]);
	if (_is_success_status(status_code)) {
		response["ok"] = true;
		response["error"] = "";
	} else {
		response["ok"] = false;
		String detail = _extract_error_message_from_body(parsed_body);
		response["error"] = detail.is_empty() ? vformat("Backend returned HTTP %d.", status_code) : detail;
	}

	print_line(vformat("[UltimateAIBackendAdapter] path=%s status=%d request_id=%s correlation_id=%s", p_path, status_code, String(response["request_id"]), String(response["correlation_id"])));

	return response;
}

Dictionary UltimateAIBackendContractAdapter::_request_json(HTTPClient::Method p_method, const String &p_path, const Variant &p_body) const {
	Dictionary response;
	int max_attempts = MAX(1, runtime_config.retry_count + 1);
	for (int attempt = 0; attempt < max_attempts; attempt++) {
		response = _request_once(p_method, p_path, p_body);
		response["attempt"] = attempt + 1;
		if (bool(response.get("ok", false))) {
			return response;
		}
		if (!_is_retryable_failure(response) || attempt + 1 >= max_attempts) {
			break;
		}
		OS::get_singleton()->delay_usec(250000);
	}
	return response;
}

void UltimateAIBackendContractAdapter::apply_runtime_config(const Dictionary &p_config) {
	if (p_config.has("base_url")) {
		runtime_config.base_url = String(p_config["base_url"]).strip_edges();
	}
	if (p_config.has("auth_mode")) {
		runtime_config.auth_mode = String(p_config["auth_mode"]).strip_edges().to_lower();
	}
	if (p_config.has("token")) {
		runtime_config.token = String(p_config["token"]).strip_edges();
	}
	if (p_config.has("token_hook")) {
		runtime_config.token_hook = String(p_config["token_hook"]).strip_edges();
	}
	if (p_config.has("actor_id")) {
		runtime_config.actor_id = String(p_config["actor_id"]).strip_edges();
	}
	if (p_config.has("tier")) {
		runtime_config.tier = String(p_config["tier"]).strip_edges();
	}
	if (p_config.has("timeout_ms")) {
		runtime_config.timeout_ms = CLAMP(int(p_config["timeout_ms"]), 1000, 120000);
	}
	if (p_config.has("retry_count")) {
		runtime_config.retry_count = CLAMP(int(p_config["retry_count"]), 0, 5);
	}
	if (p_config.has("require_signed_commands")) {
		runtime_config.require_signed_commands = bool(p_config["require_signed_commands"]);
	}
	if (p_config.has("command_allowlist")) {
		Variant allowlist = p_config["command_allowlist"];
		if (allowlist.get_type() == Variant::PACKED_STRING_ARRAY) {
			command_allowlist = allowlist;
		}
	}

	if (command_allowlist.is_empty()) {
		command_allowlist.push_back("chat_message");
	}
}

Dictionary UltimateAIBackendContractAdapter::get_runtime_config() const {
	Dictionary config;
	config["base_url"] = runtime_config.base_url;
	config["auth_mode"] = runtime_config.auth_mode;
	config["token"] = runtime_config.token;
	config["token_hook"] = runtime_config.token_hook;
	config["actor_id"] = runtime_config.actor_id;
	config["tier"] = runtime_config.tier;
	config["timeout_ms"] = runtime_config.timeout_ms;
	config["retry_count"] = runtime_config.retry_count;
	config["require_signed_commands"] = runtime_config.require_signed_commands;
	config["command_allowlist"] = command_allowlist;
	return config;
}

Dictionary UltimateAIBackendContractAdapter::start_session(const Dictionary &p_payload) const {
	return _request_json(HTTPClient::METHOD_POST, "/api/v1/session/start", p_payload);
}

Dictionary UltimateAIBackendContractAdapter::request_task(const Dictionary &p_payload) const {
	return _request_json(HTTPClient::METHOD_POST, "/api/v1/task/request", p_payload);
}

Dictionary UltimateAIBackendContractAdapter::get_task_status(const String &p_plan_id) const {
	return _request_json(HTTPClient::METHOD_GET, "/api/v1/task/" + p_plan_id);
}

Dictionary UltimateAIBackendContractAdapter::submit_approval(const String &p_plan_id, const Dictionary &p_payload) const {
	return _request_json(HTTPClient::METHOD_POST, "/api/v1/task/" + p_plan_id + "/approval", p_payload);
}

Dictionary UltimateAIBackendContractAdapter::auth_handshake(const Dictionary &p_payload) const {
	return _request_json(HTTPClient::METHOD_POST, "/api/v1/auth/handshake", p_payload);
}

Dictionary UltimateAIBackendContractAdapter::list_tools() const {
	return _request_json(HTTPClient::METHOD_GET, "/api/v1/tools");
}

Dictionary UltimateAIBackendContractAdapter::invoke_tool(const Dictionary &p_payload) const {
	return _request_json(HTTPClient::METHOD_POST, "/api/v1/tools/invoke", p_payload);
}

bool UltimateAIBackendContractAdapter::_allowlist_contains(const String &p_action) const {
	for (int i = 0; i < command_allowlist.size(); i++) {
		if (command_allowlist[i] == p_action) {
			return true;
		}
	}
	return false;
}

Dictionary UltimateAIBackendContractAdapter::evaluate_command_trust(const Dictionary &p_command) const {
	Dictionary trust;
	trust["allowed"] = false;
	trust["trusted"] = false;
	trust["reason"] = "command_not_allowed";

	if (!p_command.has("action")) {
		trust["reason"] = "missing_action";
		return trust;
	}

	String action = String(p_command["action"]);
	if (action.is_empty()) {
		trust["reason"] = "empty_action";
		return trust;
	}

	if (!_allowlist_contains(action)) {
		trust["reason"] = "action_not_in_allowlist";
		return trust;
	}

	trust["allowed"] = true;

	if (!runtime_config.require_signed_commands || action == "chat_message") {
		trust["trusted"] = true;
		trust["reason"] = "trusted";
		return trust;
	}

	String signature = String(p_command.get("signature", String()));
	if (signature.is_empty()) {
		trust["reason"] = "missing_signature";
		return trust;
	}

	String signing_token = _resolve_auth_token();
	if (signing_token.is_empty()) {
		trust["reason"] = "missing_signing_token";
		return trust;
	}

	Dictionary canonical_cmd = p_command;
	canonical_cmd.erase("signature");
	String canonical = JSON::stringify(canonical_cmd, "", true, true);
	String expected_signature = (canonical + "|" + signing_token).sha256_text();

	if (expected_signature != signature) {
		trust["reason"] = "invalid_signature";
		return trust;
	}

	trust["trusted"] = true;
	trust["reason"] = "trusted";
	return trust;
}

PackedStringArray UltimateAIBackendContractAdapter::get_command_allowlist() const {
	return command_allowlist;
}

void UltimateAIBackendContractAdapter::set_command_allowlist(const PackedStringArray &p_allowlist) {
	command_allowlist = p_allowlist;
	if (command_allowlist.is_empty()) {
		command_allowlist.push_back("chat_message");
	}
}
