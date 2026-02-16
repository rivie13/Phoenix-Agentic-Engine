/**************************************************************************/
/*  bfxr_runtime_bridge.h                                                 */
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

#include "core/object/ref_counted.h"

class BfxrRuntimeBridge : public RefCounted {
	GDCLASS(BfxrRuntimeBridge, RefCounted);

	mutable String last_error;

	Dictionary _run_bridge_command(const String &p_command, const Dictionary &p_args = Dictionary()) const;
	String _find_node_path() const;
	String _find_bridge_script_path() const;
	void _set_last_error(const String &p_error) const;
	bool _is_node_invocable(const String &p_node_path) const;

protected:
	static void _bind_methods();

public:
	bool is_runtime_available() const;
	String get_last_error() const;

	Dictionary list_synths() const;
	Dictionary list_presets(const String &p_synth = "bfxr") const;
	Dictionary list_params(const String &p_synth = "bfxr") const;
	Dictionary generate_wav(const Dictionary &p_options) const;
};
