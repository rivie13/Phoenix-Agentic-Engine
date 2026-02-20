/**************************************************************************/
/*  backend_contract_adapter.h                                            */
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

#pragma once

#include "core/io/http_client.h"
#include "core/object/object.h"

class UltimateAIBackendContractAdapter : public Object {
	GDCLASS(UltimateAIBackendContractAdapter, Object);

	struct RuntimeConfig {
		String base_url = "";
		String auth_mode = "managed";
		String token = "";
		String token_hook = "";
		String actor_id = "";
		String tier = "";
		int timeout_ms = 15000;
		int retry_count = 1;
		bool require_signed_commands = true;
		bool allow_background_agents = true;
		bool auto_approve_reads = true;
		bool require_approvals = true;
		bool mcp_enabled = true;
		bool tool_godot_mcp_docs_enabled = true;
		bool tool_godot_mcp_enabled = true;
		bool tool_godot_copilot_enabled = true;
		bool tool_autonomous_primitives_enabled = true;
		String mcp_transport = "stdio";
		bool mcp_auto_discover = true;
		bool mcp_require_approvals = true;
		String mcp_config_path = "";
	};

	RuntimeConfig runtime_config;
	PackedStringArray command_allowlist;

	String _now_iso8601_utc() const;
	String _generate_request_id(const String &p_prefix) const;
	String _resolve_auth_token() const;
	String _extract_header_value(const List<String> &p_headers, const String &p_key) const;
	String _extract_error_message_from_body(const Variant &p_body) const;
	bool _is_retryable_failure(const Dictionary &p_response) const;
	Dictionary _request_once(HTTPClient::Method p_method, const String &p_path, const Variant &p_body) const;

	Dictionary _request_json(HTTPClient::Method p_method, const String &p_path, const Variant &p_body = Variant()) const;

	bool _allowlist_contains(const String &p_action) const;

protected:
	static void _bind_methods();

public:
	void apply_runtime_config(const Dictionary &p_config);
	Dictionary get_runtime_config() const;

	Dictionary start_session(const Dictionary &p_payload) const;
	Dictionary send_session_delta(const Dictionary &p_payload) const;
	Dictionary request_task(const Dictionary &p_payload) const;
	Dictionary get_task_status(const String &p_plan_id) const;
	Dictionary submit_approval(const String &p_plan_id, const Dictionary &p_payload) const;
	Dictionary auth_handshake(const Dictionary &p_payload) const;
	Dictionary list_tools() const;
	Dictionary invoke_tool(const Dictionary &p_payload) const;
	Dictionary realtime_negotiate(const Dictionary &p_payload) const;
	Dictionary realtime_join(const Dictionary &p_payload) const;
	Dictionary list_locks(const String &p_session_id = String()) const;
	Dictionary release_lock(const String &p_lock_id) const;

	Dictionary evaluate_command_trust(const Dictionary &p_command) const;
	PackedStringArray get_command_allowlist() const;
	void set_command_allowlist(const PackedStringArray &p_allowlist);

	UltimateAIBackendContractAdapter();
};
