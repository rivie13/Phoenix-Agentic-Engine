/**************************************************************************/
/*  assistant_panel.cpp                                                   */
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

#include "assistant_panel.h"

#include "core/object/class_db.h"
#include "core/string/ustring.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"

void UltimateAssistantPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_send_pressed"), &UltimateAssistantPanel::_on_send_pressed);
	ClassDB::bind_method(D_METHOD("_on_input_submitted", "text"), &UltimateAssistantPanel::_on_input_submitted);
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
	title_label->set_text(TTR("Assistant"));
	title_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	header->add_child(title_label);

	mode_selector = memnew(OptionButton);
	mode_selector->add_item(TTR("Ask"));
	mode_selector->add_item(TTR("Plan"));
	mode_selector->add_item(TTR("Agent"));
	mode_selector->set_tooltip_text(TTR("Mode (stub)"));
	header->add_child(mode_selector);

	model_selector = memnew(OptionButton);
	model_selector->add_item(TTR("Select model"));
	model_selector->add_item(TTR("Claude"));
	model_selector->add_item(TTR("GPT-5.2-Codex"));
	model_selector->add_item(TTR("Local"));
	model_selector->set_item_disabled(0, true);
	model_selector->set_tooltip_text(TTR("Model (stub)"));
	header->add_child(model_selector);

	root->add_child(memnew(HSeparator));

	scroll_container = memnew(ScrollContainer);
	scroll_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(scroll_container);

	chat_display = memnew(RichTextLabel);
	chat_display->set_use_bbcode(true);
	chat_display->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	chat_display->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_display->set_text(TTR("[b]Assistant UI ready.[/b]\n"));
	scroll_container->add_child(chat_display);

	HBoxContainer *input_row = memnew(HBoxContainer);
	input_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(input_row);

	input_line = memnew(LineEdit);
	input_line->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_line->set_placeholder(TTR("Type a message..."));
	input_line->connect(SceneStringName(text_submitted), callable_mp(this, &UltimateAssistantPanel::_on_input_submitted));
	input_row->add_child(input_line);

	send_button = memnew(Button);
	send_button->set_text(TTR("Send"));
	send_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_send_pressed));
	input_row->add_child(send_button);
}

void UltimateAssistantPanel::_append_message(const String &p_role, const String &p_content) {
	String safe_role = p_role.strip_edges();
	String safe_content = p_content.strip_edges();
	if (safe_content.is_empty()) {
		return;
	}

	chat_display->append_text("[b]" + safe_role + ":[/b] " + safe_content + "\n");
	chat_display->scroll_to_line(chat_display->get_line_count());
}

void UltimateAssistantPanel::_on_send_pressed() {
	if (!input_line) {
		return;
	}
	_on_input_submitted(input_line->get_text());
}

void UltimateAssistantPanel::_on_input_submitted(const String &p_text) {
	String trimmed = p_text.strip_edges();
	if (trimmed.is_empty()) {
		return;
	}
	_append_message(TTR("User"), trimmed);
	input_line->clear();
}
