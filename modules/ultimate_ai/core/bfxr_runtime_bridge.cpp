/**************************************************************************/
/*  bfxr_runtime_bridge.cpp                                               */
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

#include "bfxr_runtime_bridge.h"

#include "core/crypto/crypto_core.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/object/class_db.h"
#include "core/os/os.h"

namespace {
const char *const BFXR_BRIDGE_SCRIPT_NAME = "phoenix_bridge.js";

String _platform_node_subdir() {
#if defined(WINDOWS_ENABLED)
	return "windows";
#elif defined(MACOS_ENABLED)
	return "macos";
#elif defined(LINUXBSD_ENABLED)
	return "linux";
#else
	return "";
#endif
}

Variant _parse_json_output(const String &p_output) {
	String stripped = p_output.strip_edges();
	if (stripped.is_empty()) {
		return Variant();
	}

	Variant parsed = JSON::parse_string(stripped);
	if (parsed.get_type() == Variant::DICTIONARY || parsed.get_type() == Variant::ARRAY) {
		return parsed;
	}

	PackedStringArray lines = stripped.split("\n", false);
	for (int i = lines.size() - 1; i >= 0; i--) {
		const String line = lines[i].strip_edges();
		if (line.is_empty()) {
			continue;
		}
		Variant line_parsed = JSON::parse_string(line);
		if (line_parsed.get_type() == Variant::DICTIONARY || line_parsed.get_type() == Variant::ARRAY) {
			return line_parsed;
		}
	}

	return Variant();
}
} //namespace

void BfxrRuntimeBridge::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_runtime_available"), &BfxrRuntimeBridge::is_runtime_available);
	ClassDB::bind_method(D_METHOD("get_last_error"), &BfxrRuntimeBridge::get_last_error);
	ClassDB::bind_method(D_METHOD("list_synths"), &BfxrRuntimeBridge::list_synths);
	ClassDB::bind_method(D_METHOD("list_presets", "synth"), &BfxrRuntimeBridge::list_presets, DEFVAL("bfxr"));
	ClassDB::bind_method(D_METHOD("list_params", "synth"), &BfxrRuntimeBridge::list_params, DEFVAL("bfxr"));
	ClassDB::bind_method(D_METHOD("generate_wav", "options"), &BfxrRuntimeBridge::generate_wav);
}

void BfxrRuntimeBridge::_set_last_error(const String &p_error) const {
	last_error = p_error;
}

String BfxrRuntimeBridge::get_last_error() const {
	return last_error;
}

bool BfxrRuntimeBridge::_is_node_invocable(const String &p_node_path) const {
	if (p_node_path.is_empty()) {
		return false;
	}

	if (p_node_path != "node" && !FileAccess::exists(p_node_path)) {
		return false;
	}

	List<String> args;
	args.push_back("--version");
	String output;
	int exit_code = 1;
	Error err = OS::get_singleton()->execute(p_node_path, args, &output, &exit_code, true);
	return err == OK && exit_code == 0;
}

bool BfxrRuntimeBridge::is_runtime_available() const {
	const String script_path = _find_bridge_script_path();
	if (script_path.is_empty()) {
		_set_last_error("BFXR bridge script not found. Expected phoenix_bridge.js in the BFXR runtime payload.");
		return false;
	}

	const String node_path = _find_node_path();
	if (!_is_node_invocable(node_path)) {
		_set_last_error("Node runtime not available. Ensure bundled node exists under tools/node or install node in PATH.");
		return false;
	}

	_set_last_error("");
	return true;
}

String BfxrRuntimeBridge::_find_node_path() const {
	const String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	Vector<String> candidates;
	const String node_subdir = _platform_node_subdir();

	if (!node_subdir.is_empty()) {
		const String node_binary =
#if defined(WINDOWS_ENABLED)
				"node.exe";
#else
				"node";
#endif
		candidates.push_back(exec_dir.path_join("tools/node").path_join(node_subdir).path_join(node_binary).simplify_path());
		candidates.push_back(exec_dir.path_join("../tools/node").path_join(node_subdir).path_join(node_binary).simplify_path());
		candidates.push_back(exec_dir.path_join("../../tools/node").path_join(node_subdir).path_join(node_binary).simplify_path());
	}

	for (int i = 0; i < candidates.size(); i++) {
		if (FileAccess::exists(candidates[i])) {
			return candidates[i];
		}
	}

	return "node";
}

String BfxrRuntimeBridge::_find_bridge_script_path() const {
	const String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	Vector<String> candidates;

	candidates.push_back(String(__FILE__).get_base_dir().path_join("../external/bfxr2-mcp-server").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(String(__FILE__).get_base_dir().path_join("../tools").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("modules/ultimate_ai/external/bfxr2-mcp-server").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("modules/ultimate_ai/tools").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("../modules/ultimate_ai/external/bfxr2-mcp-server").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("../modules/ultimate_ai/tools").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("../../modules/ultimate_ai/external/bfxr2-mcp-server").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("../../modules/ultimate_ai/tools").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("../../../modules/ultimate_ai/external/bfxr2-mcp-server").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("../../../modules/ultimate_ai/tools").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("addons/bfxr2-mcp-server").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());
	candidates.push_back(exec_dir.path_join("../addons/bfxr2-mcp-server").path_join(BFXR_BRIDGE_SCRIPT_NAME).simplify_path());

	for (int i = 0; i < candidates.size(); i++) {
		if (FileAccess::exists(candidates[i])) {
			return candidates[i];
		}
	}

	return "";
}

Dictionary BfxrRuntimeBridge::_run_bridge_command(const String &p_command, const Dictionary &p_args) const {
	Dictionary response;
	response["ok"] = false;
	response["command"] = p_command;

	if (!is_runtime_available()) {
		response["error"] = get_last_error();
		return response;
	}

	Dictionary payload;
	payload["command"] = p_command;
	payload["args"] = p_args;
	const String payload_json = JSON::stringify(payload);
	const CharString payload_utf8 = payload_json.utf8();
	const String payload_base64 = CryptoCore::b64_encode_str((const uint8_t *)payload_utf8.get_data(), payload_utf8.length());

	List<String> args;
	args.push_back(_find_bridge_script_path());
	args.push_back("--json-base64");
	args.push_back(payload_base64);

	String output;
	int exit_code = 1;
	Error exec_err = OS::get_singleton()->execute(_find_node_path(), args, &output, &exit_code, true);
	if (exec_err != OK) {
		String err = vformat("BFXR bridge execution failed with error code %s.", itos(exec_err));
		_set_last_error(err);
		response["error"] = err;
		response["stderr"] = output;
		return response;
	}

	if (exit_code != 0) {
		String err = vformat("BFXR bridge process exited with code %s.", itos(exit_code));
		_set_last_error(err);
		response["error"] = err;
		response["stderr"] = output;
		return response;
	}

	Variant parsed = _parse_json_output(output);
	if (parsed.get_type() != Variant::DICTIONARY) {
		String err = "BFXR bridge returned invalid JSON payload.";
		_set_last_error(err);
		response["error"] = err;
		response["stdout"] = output;
		return response;
	}

	Dictionary parsed_dict = parsed;
	if (!parsed_dict.has("ok")) {
		parsed_dict["ok"] = true;
	}
	if (bool(parsed_dict["ok"])) {
		_set_last_error("");
	} else {
		_set_last_error(parsed_dict.get("error", "Unknown bridge error"));
	}
	return parsed_dict;
}

Dictionary BfxrRuntimeBridge::list_synths() const {
	return _run_bridge_command("list_synths");
}

Dictionary BfxrRuntimeBridge::list_presets(const String &p_synth) const {
	Dictionary args;
	args["synth"] = p_synth;
	return _run_bridge_command("list_presets", args);
}

Dictionary BfxrRuntimeBridge::list_params(const String &p_synth) const {
	Dictionary args;
	args["synth"] = p_synth;
	return _run_bridge_command("list_params", args);
}

Dictionary BfxrRuntimeBridge::generate_wav(const Dictionary &p_options) const {
	return _run_bridge_command("generate_wav", p_options);
}
