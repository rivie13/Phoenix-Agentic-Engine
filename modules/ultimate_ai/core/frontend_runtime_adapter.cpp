/**************************************************************************/
/*  frontend_runtime_adapter.cpp                                          */
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

#include "frontend_runtime_adapter.h"

#include "backend_contract_adapter.h"

Dictionary UltimateAIFrontendRuntimeAdapter::negotiate_realtime(UltimateAIBackendContractAdapter *p_backend_adapter, const String &p_session_id, const String &p_user_id) const {
	Dictionary result;
	result["ok"] = false;
	result["error"] = "Backend adapter unavailable.";

	if (!p_backend_adapter) {
		return result;
	}

	String session_id = p_session_id.strip_edges();
	String user_id = p_user_id.strip_edges();
	if (session_id.is_empty() || user_id.is_empty()) {
		result["error"] = "session_id and user_id are required for realtime negotiation.";
		return result;
	}

	Dictionary payload;
	payload["schema_version"] = "v1";
	payload["session_id"] = session_id;
	payload["user_id"] = user_id;

	Dictionary response = p_backend_adapter->realtime_negotiate(payload);
	result = response;

	if (!bool(response.get("ok", false))) {
		return result;
	}

	Variant body_v = response.get("body", Variant());
	if (body_v.get_type() != Variant::DICTIONARY) {
		result["ok"] = false;
		result["error"] = "Realtime negotiate response body was not a JSON object.";
		return result;
	}

	Dictionary body = body_v;
	if (String(body.get("schema_version", String())) != "v1") {
		result["ok"] = false;
		result["error"] = "Realtime negotiate response schema_version mismatch.";
		return result;
	}
	if (String(body.get("event", String())) != "realtime_negotiate_ack") {
		result["ok"] = false;
		result["error"] = "Realtime negotiate response event mismatch.";
		return result;
	}
	if (String(body.get("session_id", String())).strip_edges() != session_id) {
		result["ok"] = false;
		result["error"] = "Realtime negotiate response session_id mismatch.";
		return result;
	}

	result["realtime_url"] = String(body.get("url", String()));
	result["realtime_access_token"] = String(body.get("access_token", String()));
	result["realtime_groups"] = body.get("groups", Array());
	result["realtime_event"] = body.get("event", String("realtime_negotiate_ack"));
	return result;
}

Dictionary UltimateAIFrontendRuntimeAdapter::map_realtime_event(const Dictionary &p_event, const String &p_expected_session_id) const {
	Dictionary mapped;
	mapped["handled"] = false;
	mapped["requires_resync"] = false;
	mapped["requires_status_refresh"] = false;
	mapped["event"] = String();
	mapped["plan_id"] = String();
	mapped["message"] = String();
	mapped["last_confirmed_seq"] = -1;
	mapped["seq"] = -1;
	mapped["chat_delta"] = String();
	mapped["chat_done"] = false;
	mapped["thinking_delta"] = String();
	mapped["thinking_done"] = false;
	mapped["chat_role"] = String("Assistant");

	String schema_version = String(p_event.get("schema_version", String())).strip_edges();
	String event_name = String(p_event.get("event", String())).strip_edges();
	if (schema_version != "v1" || event_name.is_empty()) {
		return mapped;
	}

	mapped["handled"] = true;
	mapped["event"] = event_name;
	mapped["seq"] = int(p_event.get("seq", -1));

	String expected_session_id = p_expected_session_id.strip_edges();
	String event_session_id = String(p_event.get("session_id", String())).strip_edges();
	String plan_id = String(p_event.get("plan_id", String())).strip_edges();
	mapped["plan_id"] = plan_id;

	if (event_name == "session.resync_required") {
		if (!expected_session_id.is_empty() && !event_session_id.is_empty() && event_session_id != expected_session_id) {
			mapped["handled"] = false;
			mapped["message"] = "Ignored session.resync_required for a different session.";
			return mapped;
		}

		mapped["requires_resync"] = true;
		mapped["last_confirmed_seq"] = int(p_event.get("last_confirmed_seq", -1));
		String reason = String(p_event.get("reason", String("state_drift"))).strip_edges();
		if (reason.is_empty()) {
			reason = "state_drift";
		}
		mapped["message"] = vformat("Realtime requested session resync (reason: %s).", reason);
		return mapped;
	}

	if (event_name == "plan.ready") {
		mapped["requires_status_refresh"] = true;
		mapped["message"] = plan_id.is_empty()
				? String("Realtime update: plan.ready received. Refresh task status.")
				: vformat("Realtime update: plan.ready for %s.", plan_id);
		return mapped;
	}

	if (event_name == "chat.delta" || event_name == "chat_delta") {
		String chunk = String(p_event.get("text", String()));
		if (chunk.strip_edges().is_empty()) {
			mapped["handled"] = false;
			return mapped;
		}
		mapped["chat_delta"] = chunk;
		mapped["chat_role"] = String(p_event.get("agent", String("Assistant"))).strip_edges();
		if (String(mapped["chat_role"]).is_empty()) {
			mapped["chat_role"] = String("Assistant");
		}
		mapped["requires_status_refresh"] = false;
		mapped["message"] = String();
		return mapped;
	}

	if (event_name == "chat.done" || event_name == "chat_done") {
		mapped["chat_done"] = true;
		mapped["message"] = String();
		return mapped;
	}

	if (event_name == "chat.thinking" || event_name == "chat_thinking") {
		String thinking_chunk = String(p_event.get("text", String()));
		if (thinking_chunk.strip_edges().is_empty()) {
			mapped["handled"] = false;
			return mapped;
		}
		mapped["thinking_delta"] = thinking_chunk;
		mapped["requires_status_refresh"] = false;
		mapped["message"] = String();
		return mapped;
	}

	if (event_name == "chat.thinking.done" || event_name == "chat_thinking_done") {
		mapped["thinking_done"] = true;
		mapped["message"] = String();
		return mapped;
	}

	if (event_name == "job.error") {
		String detail = String(p_event.get("message", String("Worker reported an error."))).strip_edges();
		mapped["message"] = detail;
		return mapped;
	}

	if (event_name == "lock.conflict") {
		String resource_path = String(p_event.get("resource_path", String())).strip_edges();
		if (resource_path.is_empty()) {
			mapped["message"] = "Realtime update: lock conflict reported.";
		} else {
			mapped["message"] = vformat("Realtime update: lock conflict on %s.", resource_path);
		}
		return mapped;
	}

	if (event_name == "job.queued" || event_name == "job.started" || event_name == "job.done" || event_name == "job.expired") {
		mapped["requires_status_refresh"] = event_name == "job.done";
		mapped["message"] = String();
		return mapped;
	}

	if (event_name == "orch.step.start" || event_name == "orch.step.log" || event_name == "orch.step.end") {
		String detail = String(p_event.get("message", String())).strip_edges();
		if (detail.is_empty()) {
			detail = String(p_event.get("description", String())).strip_edges();
		}
		mapped["message"] = detail.is_empty() ? vformat("Realtime update: %s.", event_name) : detail;
		return mapped;
	}

	mapped["handled"] = false;
	mapped["message"] = String();
	return mapped;
}
