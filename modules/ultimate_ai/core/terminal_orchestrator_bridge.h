/**************************************************************************/
/*  terminal_orchestrator_bridge.h                                        */
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

#pragma once

#include "core/object/object.h"

class UltimateAITerminalBridge : public Object {
	GDCLASS(UltimateAITerminalBridge, Object);

	Object *_get_terminal_api_target() const;

protected:
	static void _bind_methods();

public:
	bool is_available() const;
	Variant execute_terminal_action(const String &p_action, const Dictionary &p_args = Dictionary()) const;

	Array list_terminals() const;
	int create_terminal(const String &p_name = "", bool p_ai_terminal = true) const;
	bool select_terminal(int p_terminal_id) const;
	bool close_terminal(int p_terminal_id) const;
	bool send_terminal_input(int p_terminal_id, const String &p_text, bool p_append_newline = true) const;
	bool send_to_active_terminal(const String &p_text, bool p_append_newline = true) const;
	bool restart_terminal(int p_terminal_id) const;
};
