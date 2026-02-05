/**************************************************************************/
/*  assistant_panel.h                                                     */
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

#include "scene/gui/panel_container.h"

class Button;
class HBoxContainer;
class Label;
class LineEdit;
class OptionButton;
class RichTextLabel;
class ScrollContainer;
class VBoxContainer;

class UltimateAssistantPanel : public PanelContainer {
	GDCLASS(UltimateAssistantPanel, PanelContainer);

	VBoxContainer *root = nullptr;
	HBoxContainer *header = nullptr;
	Label *title_label = nullptr;
	OptionButton *mode_selector = nullptr;
	OptionButton *model_selector = nullptr;
	ScrollContainer *scroll_container = nullptr;
	RichTextLabel *chat_display = nullptr;
	LineEdit *input_line = nullptr;
	Button *send_button = nullptr;

	void _on_send_pressed();
	void _on_input_submitted(const String &p_text);
	void _append_message(const String &p_role, const String &p_content);

protected:
	static void _bind_methods();

public:
	UltimateAssistantPanel();
};
