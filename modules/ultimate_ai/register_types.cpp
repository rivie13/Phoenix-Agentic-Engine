/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#ifdef TOOLS_ENABLED
#include "core/backend_contract_adapter.h"
#include "core/bfxr_runtime_bridge.h"
#include "core/object/class_db.h"
#include "core/terminal_orchestrator_bridge.h"
#include "ui/bfxr_editor_plugin.h"
#include "ui/diff_margin_editor_plugin.h"
#include "ui/gdterm_editor_plugin.h"
#include "ui/git_plugin_editor_plugin.h"
#include "ui/gut_editor_plugin.h"
#include "ui/pixelpen_editor_plugin.h"
#include "ui/ultimate_ai_editor_plugin.h"
#endif

void initialize_ultimate_ai_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
		return;
	}

#ifdef TOOLS_ENABLED
	ClassDB::register_class<BfxrRuntimeBridge>();
	ClassDB::register_class<UltimateAIBackendContractAdapter>();
	ClassDB::register_class<UltimateAITerminalBridge>();
	EditorPlugins::add_by_type<UltimateAIEditorPlugin>();
	EditorPlugins::add_by_type<BfxrEditorPlugin>();
	EditorPlugins::add_by_type<DiffMarginEditorPlugin>();
	EditorPlugins::add_by_type<GDTermEditorPlugin>();
	EditorPlugins::add_by_type<GitPluginEditorPlugin>();
	EditorPlugins::add_by_type<GutEditorPlugin>();
	EditorPlugins::add_by_type<PixelPenEditorPlugin>();
#endif
}

void uninitialize_ultimate_ai_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
		return;
	}

	// TODO: Unregister classes for the Ultimate AI module.
}
