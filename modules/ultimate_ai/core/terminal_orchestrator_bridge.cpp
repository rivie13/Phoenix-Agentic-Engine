/**************************************************************************/
/*  terminal_orchestrator_bridge.cpp                                      */
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

#include "terminal_orchestrator_bridge.h"

#include "core/object/class_db.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"

namespace {
const char *const GDTERM_PLUGIN_GROUP = "gdterm_plugin";
const char *const GDTERM_MANAGER_GROUP = "gdterm_terminal_manager";

bool _variant_to_bool(const Variant &p_variant) {
	switch (p_variant.get_type()) {
		case Variant::BOOL:
			return p_variant;
		case Variant::INT:
			return int64_t(p_variant) != 0;
		default:
			return false;
	}
}
} //namespace

void UltimateAITerminalBridge::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_available"), &UltimateAITerminalBridge::is_available);
	ClassDB::bind_method(D_METHOD("execute_terminal_action", "action", "args"), &UltimateAITerminalBridge::execute_terminal_action, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("list_terminals"), &UltimateAITerminalBridge::list_terminals);
	ClassDB::bind_method(D_METHOD("create_terminal", "name", "ai_terminal"), &UltimateAITerminalBridge::create_terminal, DEFVAL(""), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("select_terminal", "terminal_id"), &UltimateAITerminalBridge::select_terminal);
	ClassDB::bind_method(D_METHOD("close_terminal", "terminal_id"), &UltimateAITerminalBridge::close_terminal);
	ClassDB::bind_method(D_METHOD("send_terminal_input", "terminal_id", "text", "append_newline"), &UltimateAITerminalBridge::send_terminal_input, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("send_to_active_terminal", "text", "append_newline"), &UltimateAITerminalBridge::send_to_active_terminal, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("restart_terminal", "terminal_id"), &UltimateAITerminalBridge::restart_terminal);
}

Object *UltimateAITerminalBridge::_get_terminal_api_target() const {
	SceneTree *tree = SceneTree::get_singleton();
	if (!tree) {
		return nullptr;
	}

	Vector<Node *> plugin_nodes = tree->get_nodes_in_group(GDTERM_PLUGIN_GROUP);
	if (!plugin_nodes.is_empty()) {
		return plugin_nodes[0];
	}

	Vector<Node *> manager_nodes = tree->get_nodes_in_group(GDTERM_MANAGER_GROUP);
	if (!manager_nodes.is_empty()) {
		return manager_nodes[0];
	}

	return nullptr;
}

bool UltimateAITerminalBridge::is_available() const {
	Object *target = _get_terminal_api_target();
	return target != nullptr && target->has_method("ai_execute");
}

Variant UltimateAITerminalBridge::execute_terminal_action(const String &p_action, const Dictionary &p_args) const {
	if (p_action.is_empty()) {
		return Variant();
	}

	Object *target = _get_terminal_api_target();
	if (!target || !target->has_method("ai_execute")) {
		return Variant();
	}

	return target->call("ai_execute", p_action, p_args);
}

Array UltimateAITerminalBridge::list_terminals() const {
	Variant result = execute_terminal_action("list");
	if (result.get_type() == Variant::ARRAY) {
		return result;
	}
	return Array();
}

int UltimateAITerminalBridge::create_terminal(const String &p_name, bool p_ai_terminal) const {
	Dictionary args;
	args["name"] = p_name;
	args["ai"] = p_ai_terminal;
	Variant result = execute_terminal_action("create", args);
	if (result.get_type() == Variant::INT) {
		return int64_t(result);
	}
	return -1;
}

bool UltimateAITerminalBridge::select_terminal(int p_terminal_id) const {
	Dictionary args;
	args["id"] = p_terminal_id;
	Variant result = execute_terminal_action("select", args);
	return _variant_to_bool(result);
}

bool UltimateAITerminalBridge::close_terminal(int p_terminal_id) const {
	Dictionary args;
	args["id"] = p_terminal_id;
	Variant result = execute_terminal_action("close", args);
	return _variant_to_bool(result);
}

bool UltimateAITerminalBridge::send_terminal_input(int p_terminal_id, const String &p_text, bool p_append_newline) const {
	Dictionary args;
	args["id"] = p_terminal_id;
	args["text"] = p_text;
	args["append_newline"] = p_append_newline;
	Variant result = execute_terminal_action("send", args);
	return _variant_to_bool(result);
}

bool UltimateAITerminalBridge::send_to_active_terminal(const String &p_text, bool p_append_newline) const {
	Dictionary args;
	args["text"] = p_text;
	args["append_newline"] = p_append_newline;
	Variant result = execute_terminal_action("send_active", args);
	return _variant_to_bool(result);
}

bool UltimateAITerminalBridge::restart_terminal(int p_terminal_id) const {
	Dictionary args;
	args["id"] = p_terminal_id;
	Variant result = execute_terminal_action("restart", args);
	return _variant_to_bool(result);
}
