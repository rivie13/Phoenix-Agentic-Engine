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

#include "assistant_settings_dialog.h"

#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/string/ustring.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"

void UltimateAssistantPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_new_tab_pressed"), &UltimateAssistantPanel::_on_new_tab_pressed);
	ClassDB::bind_method(D_METHOD("_on_tab_close_requested", "tab_index"), &UltimateAssistantPanel::_on_tab_close_requested);
	ClassDB::bind_method(D_METHOD("_on_send_pressed", "tab_id"), &UltimateAssistantPanel::_on_send_pressed);
	ClassDB::bind_method(D_METHOD("_on_input_submitted", "text", "tab_id"), &UltimateAssistantPanel::_on_input_submitted);
	ClassDB::bind_method(D_METHOD("_on_interrupt_pressed", "tab_id"), &UltimateAssistantPanel::_on_interrupt_pressed);
	ClassDB::bind_method(D_METHOD("_on_steer_pressed", "tab_id"), &UltimateAssistantPanel::_on_steer_pressed);
	ClassDB::bind_method(D_METHOD("_on_settings_pressed"), &UltimateAssistantPanel::_on_settings_pressed);
	ClassDB::bind_method(D_METHOD("_on_settings_confirmed"), &UltimateAssistantPanel::_on_settings_confirmed);
	ClassDB::bind_method(D_METHOD("_on_tab_setting_changed", "tab_id"), &UltimateAssistantPanel::_on_tab_setting_changed);
	ClassDB::bind_method(D_METHOD("_on_context_add_pressed", "tab_id"), &UltimateAssistantPanel::_on_context_add_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_remove_pressed", "tab_id"), &UltimateAssistantPanel::_on_context_remove_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_select_add_pressed"), &UltimateAssistantPanel::_on_context_select_add_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_select_remove_pressed"), &UltimateAssistantPanel::_on_context_select_remove_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_note_add_pressed"), &UltimateAssistantPanel::_on_context_note_add_pressed);
	ClassDB::bind_method(D_METHOD("_on_context_search_changed", "text"), &UltimateAssistantPanel::_on_context_search_changed);
	ClassDB::bind_method(D_METHOD("_on_context_toggle_toggled", "pressed", "tab_id"), &UltimateAssistantPanel::_on_context_toggle_toggled);
	ClassDB::bind_method(D_METHOD("_on_context_dialog_confirmed"), &UltimateAssistantPanel::_on_context_dialog_confirmed);
	ClassDB::bind_method(D_METHOD("_on_previous_reopen_pressed"), &UltimateAssistantPanel::_on_previous_reopen_pressed);
	ClassDB::bind_method(D_METHOD("_on_previous_session_activated", "index"), &UltimateAssistantPanel::_on_previous_session_activated);
	ClassDB::bind_method(D_METHOD("_on_session_name_changed", "text", "tab_id"), &UltimateAssistantPanel::_on_session_name_changed);
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
	title_label->set_text(TTR("Phoenix Copilot"));
	title_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	header->add_child(title_label);

	new_tab_button = memnew(Button);
	new_tab_button->set_text(TTR("New Chat"));
	new_tab_button->set_tooltip_text(TTR("Open a new assistant tab"));
	new_tab_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_new_tab_pressed));
	header->add_child(new_tab_button);

	settings_button = memnew(Button);
	settings_button->set_text(TTR("Settings"));
	settings_button->set_tooltip_text(TTR("Open assistant settings"));
	settings_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_settings_pressed));
	header->add_child(settings_button);

	root->add_child(memnew(HSeparator));

	tab_container = memnew(TabContainer);
	tab_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	TabBar *tab_bar = tab_container->get_tab_bar();
	if (tab_bar) {
		tab_bar->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_NEVER);
		tab_bar->connect("tab_button_pressed", callable_mp(this, &UltimateAssistantPanel::_on_tab_close_requested));
	}
	root->add_child(tab_container);

	settings_dialog = memnew(UltimateAISettingsDialog);
	settings_dialog->connect(SceneStringName(confirmed), callable_mp(this, &UltimateAssistantPanel::_on_settings_confirmed));
	add_child(settings_dialog);

	context_dialog = memnew(AcceptDialog);
	context_dialog->set_title(TTR("Add Context"));
	context_dialog->set_ok_button_text(TTR("Add"));
	context_dialog->connect(SceneStringName(confirmed), callable_mp(this, &UltimateAssistantPanel::_on_context_dialog_confirmed));
	add_child(context_dialog);

	VBoxContainer *context_dialog_root = memnew(VBoxContainer);
	context_dialog->add_child(context_dialog_root);

	Label *context_search_label = memnew(Label);
	context_search_label->set_text(TTR("Project files"));
	context_dialog_root->add_child(context_search_label);

	context_search_input = memnew(LineEdit);
	context_search_input->set_placeholder(TTR("Search files"));
	context_search_input->connect(SceneStringName(text_changed), callable_mp(this, &UltimateAssistantPanel::_on_context_search_changed));
	context_dialog_root->add_child(context_search_input);

	context_file_list = memnew(ItemList);
	context_file_list->set_select_mode(ItemList::SELECT_MULTI);
	context_file_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	context_dialog_root->add_child(context_file_list);

	HBoxContainer *context_pick_buttons = memnew(HBoxContainer);
	context_dialog_root->add_child(context_pick_buttons);

	context_select_add_button = memnew(Button);
	context_select_add_button->set_text(TTR("Add Selected"));
	context_select_add_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_select_add_pressed));
	context_pick_buttons->add_child(context_select_add_button);

	context_select_remove_button = memnew(Button);
	context_select_remove_button->set_text(TTR("Remove Selected"));
	context_select_remove_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_select_remove_pressed));
	context_pick_buttons->add_child(context_select_remove_button);

	Label *context_selected_label = memnew(Label);
	context_selected_label->set_text(TTR("Added context"));
	context_dialog_root->add_child(context_selected_label);

	context_selected_list = memnew(ItemList);
	context_selected_list->set_select_mode(ItemList::SELECT_MULTI);
	context_selected_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	context_dialog_root->add_child(context_selected_list);

	HBoxContainer *context_note_row = memnew(HBoxContainer);
	context_dialog_root->add_child(context_note_row);

	context_note_input = memnew(LineEdit);
	context_note_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	context_note_input->set_placeholder(TTR("Add note"));
	context_note_row->add_child(context_note_input);

	context_note_add_button = memnew(Button);
	context_note_add_button->set_text(TTR("Add Note"));
	context_note_add_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_note_add_pressed));
	context_note_row->add_child(context_note_add_button);

	_ensure_default_models();
	_build_hub_tab();
	_add_chat_tab();
	tab_container->set_current_tab(0);
}

void UltimateAssistantPanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE:
		case NOTIFICATION_THEME_CHANGED: {
			theme_ready = true;
			_refresh_tab_close_icons();
		} break;
		default:
			break;
	}
}

void UltimateAssistantPanel::_ensure_default_models() {
	if (!available_models.is_empty()) {
		return;
	}
	available_models.push_back("gpt-5.2-codex");
	available_models.push_back("claude-sonnet");
	available_models.push_back("claude-haiku");
	available_models.push_back("local-default");
}

void UltimateAssistantPanel::_refresh_model_selectors() {
	for (int i = 0; i < tabs.size(); i++) {
		OptionButton *selector = tabs[i].model_selector;
		if (!selector) {
			continue;
		}
		int selected = selector->get_selected();
		String previous = selected >= 0 ? selector->get_item_text(selected) : "";
		selector->clear();
		for (int j = 0; j < available_models.size(); j++) {
			selector->add_item(available_models[j]);
		}
		if (selector->get_item_count() > 0) {
			int match_index = -1;
			int item_count = selector->get_item_count();
			for (int k = 0; k < item_count; k++) {
				if (selector->get_item_text(k) == previous) {
					match_index = k;
					break;
				}
			}
			selector->select(match_index >= 0 ? match_index : 0);
		}
	}
	_refresh_hub();
}

void UltimateAssistantPanel::_build_hub_tab() {
	hub_root = memnew(VBoxContainer);
	hub_root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	HBoxContainer *hub_header = memnew(HBoxContainer);
	hub_header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->add_child(hub_header);

	Label *hub_title = memnew(Label);
	hub_title->set_text(TTR("Agent Hub"));
	hub_title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hub_header->add_child(hub_title);

	hub_root->add_child(memnew(HSeparator));

	Label *hub_list_label = memnew(Label);
	hub_list_label->set_text(TTR("Active agents"));
	hub_root->add_child(hub_list_label);

	hub_agent_list = memnew(ItemList);
	hub_agent_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->add_child(hub_agent_list);

	hub_root->add_child(memnew(HSeparator));

	Label *previous_label = memnew(Label);
	previous_label->set_text(TTR("Previous sessions"));
	hub_root->add_child(previous_label);

	hub_previous_list = memnew(ItemList);
	hub_previous_list->set_select_mode(ItemList::SELECT_SINGLE);
	hub_previous_list->set_allow_reselect(true);
	hub_previous_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hub_previous_list->connect("item_activated", callable_mp(this, &UltimateAssistantPanel::_on_previous_session_activated));
	hub_root->add_child(hub_previous_list);

	hub_reopen_button = memnew(Button);
	hub_reopen_button->set_text(TTR("Reopen Session"));
	hub_reopen_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_previous_reopen_pressed));
	hub_root->add_child(hub_reopen_button);

	hub_root->add_child(memnew(HSeparator));

	Label *pending_label = memnew(Label);
	pending_label->set_text(TTR("Approvals & pending actions"));
	hub_root->add_child(pending_label);

	hub_pending_list = memnew(ItemList);
	hub_pending_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->add_child(hub_pending_list);

	hub_root->add_child(memnew(HSeparator));

	Label *questions_label = memnew(Label);
	questions_label->set_text(TTR("Agent questions"));
	hub_root->add_child(questions_label);

	hub_questions_list = memnew(ItemList);
	hub_questions_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hub_root->add_child(hub_questions_list);

	tab_container->add_child(hub_root);
	tab_container->set_tab_title(tab_container->get_tab_count() - 1, TTR("Hub"));
	_refresh_tab_close_icons();
}

void UltimateAssistantPanel::_add_chat_tab() {
	ChatTab tab;
	VBoxContainer *tab_root = memnew(VBoxContainer);
	tab_root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab_root->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	OptionButton *mode_selector = memnew(OptionButton);
	mode_selector->add_item(TTR("Ask"));
	mode_selector->add_item(TTR("Plan"));
	mode_selector->add_item(TTR("Agent"));
	mode_selector->set_tooltip_text(TTR("Mode (stub)"));

	OptionButton *model_selector = memnew(OptionButton);
	for (int i = 0; i < available_models.size(); i++) {
		model_selector->add_item(available_models[i]);
	}
	if (model_selector->get_item_count() > 0) {
		model_selector->select(0);
	}
	model_selector->set_tooltip_text(TTR("Model (per chat)"));

	OptionButton *agent_mode_selector = memnew(OptionButton);
	agent_mode_selector->add_item(TTR("Local Agent"));
	agent_mode_selector->add_item(TTR("Background Agent"));
	agent_mode_selector->set_tooltip_text(TTR("Execution target (stub)"));

	LineEdit *session_name_input = memnew(LineEdit);
	session_name_input->set_placeholder(TTR("Session name"));
	session_name_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);

	tab_root->add_child(memnew(HSeparator));

	VSplitContainer *chat_split = memnew(VSplitContainer);
	chat_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	chat_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_split->set_split_offset(350);
	tab_root->add_child(chat_split);

	ScrollContainer *scroll_container = memnew(ScrollContainer);
	scroll_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_split->add_child(scroll_container);

	RichTextLabel *chat_display = memnew(RichTextLabel);
	chat_display->set_use_bbcode(true);
	chat_display->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	chat_display->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_display->set_text(TTR("[b]New chat ready.[/b]\n"));
	scroll_container->add_child(chat_display);

	VBoxContainer *input_block = memnew(VBoxContainer);
	input_block->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_block->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_split->add_child(input_block);

	HBoxContainer *controls_row = memnew(HBoxContainer);
	controls_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_block->add_child(controls_row);
	controls_row->add_child(mode_selector);
	controls_row->add_child(model_selector);
	controls_row->add_child(agent_mode_selector);
	controls_row->add_child(session_name_input);

	VBoxContainer *context_section = memnew(VBoxContainer);
	context_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_block->add_child(context_section);

	HBoxContainer *context_row = memnew(HBoxContainer);
	context_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	context_section->add_child(context_row);

	Button *context_toggle = memnew(Button);
	context_toggle->set_text(TTR("Context"));
	context_toggle->set_toggle_mode(true);
	context_toggle->set_pressed(true);
	context_row->add_child(context_toggle);

	Button *context_add_button = memnew(Button);
	context_add_button->set_text(TTR("Add"));
	context_row->add_child(context_add_button);

	Button *context_remove_button = memnew(Button);
	context_remove_button->set_text(TTR("Remove"));
	context_row->add_child(context_remove_button);

	ItemList *context_list = memnew(ItemList);
	context_list->set_select_mode(ItemList::SELECT_MULTI);
	context_list->set_v_size_flags(Control::SIZE_FILL);
	context_list->set_custom_minimum_size(Size2(0, 80));
	context_section->add_child(context_list);

	TextEdit *input_text = memnew(TextEdit);
	input_text->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_text->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	input_text->set_placeholder(TTR("Type a message..."));
	input_text->set_custom_minimum_size(Size2(0, 90));
	input_block->add_child(input_text);

	HBoxContainer *input_actions = memnew(HBoxContainer);
	input_actions->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_block->add_child(input_actions);

	Control *actions_spacer = memnew(Control);
	actions_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_actions->add_child(actions_spacer);

	Button *send_button = memnew(Button);
	send_button->set_text(TTR("Send"));
	input_actions->add_child(send_button);

	Button *steer_button = memnew(Button);
	steer_button->set_text(TTR("Steer"));
	steer_button->set_visible(false);
	input_actions->add_child(steer_button);

	int tab_id = ++tab_counter;
	send_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_send_pressed).bind(tab_id));
	steer_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_steer_pressed).bind(tab_id));
	mode_selector->connect(SceneStringName(item_selected), callable_mp(this, &UltimateAssistantPanel::_on_tab_setting_changed).bind(tab_id));
	model_selector->connect(SceneStringName(item_selected), callable_mp(this, &UltimateAssistantPanel::_on_tab_setting_changed).bind(tab_id));
	agent_mode_selector->connect(SceneStringName(item_selected), callable_mp(this, &UltimateAssistantPanel::_on_tab_setting_changed).bind(tab_id));
	context_add_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_add_pressed).bind(tab_id));
	context_remove_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAssistantPanel::_on_context_remove_pressed).bind(tab_id));
	context_toggle->connect(SceneStringName(toggled), callable_mp(this, &UltimateAssistantPanel::_on_context_toggle_toggled).bind(tab_id));
	session_name_input->connect(SceneStringName(text_changed), callable_mp(this, &UltimateAssistantPanel::_on_session_name_changed).bind(tab_id));

	String tab_title = vformat("Agent %d", tab_id);
	tab_container->add_child(tab_root);
	tab_container->set_tab_title(tab_container->get_tab_count() - 1, tab_title);

	tab.id = tab_id;
	tab.root = tab_root;
	tab.mode_selector = mode_selector;
	tab.model_selector = model_selector;
	tab.agent_mode_selector = agent_mode_selector;
	tab.session_name_input = session_name_input;
	tab.context_list = context_list;
	tab.context_section = context_section;
	tab.context_toggle_button = context_toggle;
	tab.context_add_button = context_add_button;
	tab.context_remove_button = context_remove_button;
	tab.chat_display = chat_display;
	tab.input_text = input_text;
	tab.send_button = send_button;
	tab.steer_button = steer_button;
	tab.transcript = chat_display->get_text();

	tabs.push_back(tab);
	tab_container->set_current_tab(tab_container->get_tab_count() - 1);
	_refresh_tab_close_icons();
	_refresh_hub();
}

void UltimateAssistantPanel::_append_message(int p_tab_id, const String &p_role, const String &p_content) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}

	String safe_role = p_role.strip_edges();
	String safe_content = p_content.strip_edges();
	if (safe_content.is_empty()) {
		return;
	}

	ChatTab &tab = tabs.write[tab_index];
	RichTextLabel *display = tab.chat_display;
	if (!display) {
		return;
	}

	String line = "[b]" + safe_role + ":[/b] " + safe_content + "\n";
	display->append_text(line);
	tab.transcript += line;
	display->scroll_to_line(display->get_line_count());
}

void UltimateAssistantPanel::_on_new_tab_pressed() {
	_add_chat_tab();
}

void UltimateAssistantPanel::_on_tab_close_requested(int p_tab_index) {
	if (p_tab_index == 0) {
		return;
	}
	if (p_tab_index < 0 || p_tab_index >= tab_container->get_tab_count()) {
		return;
	}
	Control *tab_control = tab_container->get_tab_control(p_tab_index);
	int tab_index = _find_tab_index_by_root(tab_control);
	if (tab_index >= 0 && tab_index < tabs.size()) {
		_archive_tab(tabs[tab_index]);
	}
	if (tab_control) {
		tab_container->remove_child(tab_control);
		tab_control->queue_free();
	}
	if (tab_index >= 0 && tab_index < tabs.size()) {
		tabs.remove_at(tab_index);
	}

	if (tab_container->get_tab_count() == 0) {
		_build_hub_tab();
		_add_chat_tab();
	}
	_refresh_tab_close_icons();
	_refresh_hub();
}

void UltimateAssistantPanel::_refresh_tab_close_icons() {
	if (!tab_container || !theme_ready) {
		return;
	}
	Ref<Texture2D> close_icon = get_theme_icon(SNAME("Close"), SNAME("EditorIcons"));
	if (!close_icon.is_valid() || close_icon->get_width() <= 0 || close_icon->get_height() <= 0) {
		return;
	}
	int tab_count = tab_container->get_tab_count();
	for (int i = 0; i < tab_count; i++) {
		if (i == 0) {
			tab_container->set_tab_button_icon(i, Ref<Texture2D>());
			continue;
		}
		tab_container->set_tab_button_icon(i, close_icon);
	}
}

int UltimateAssistantPanel::_find_tab_index_by_id(int p_tab_id) const {
	for (int i = 0; i < tabs.size(); i++) {
		if (tabs[i].id == p_tab_id) {
			return i;
		}
	}
	return -1;
}

int UltimateAssistantPanel::_find_tab_index_by_root(Control *p_root) const {
	if (!p_root) {
		return -1;
	}
	for (int i = 0; i < tabs.size(); i++) {
		if (tabs[i].root == p_root) {
			return i;
		}
	}
	return -1;
}

int UltimateAssistantPanel::_find_tab_container_index(Control *p_root) const {
	if (!tab_container || !p_root) {
		return -1;
	}
	int tab_count = tab_container->get_tab_count();
	for (int i = 0; i < tab_count; i++) {
		if (tab_container->get_tab_control(i) == p_root) {
			return i;
		}
	}
	return -1;
}

void UltimateAssistantPanel::_on_send_pressed(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	if (tab.is_active) {
		_on_interrupt_pressed(p_tab_id);
		return;
	}
	TextEdit *input = tab.input_text;
	if (!input) {
		return;
	}
	_on_input_submitted(input->get_text(), p_tab_id);
	tab.is_active = true;
	if (tab.send_button) {
		tab.send_button->set_text(TTR("Interrupt"));
	}
	if (tab.steer_button) {
		tab.steer_button->set_visible(true);
	}
}

void UltimateAssistantPanel::_on_input_submitted(const String &p_text, int p_tab_id) {
	String trimmed = p_text.strip_edges();
	if (trimmed.is_empty()) {
		return;
	}
	_append_message(p_tab_id, TTR("User"), trimmed);
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index >= 0 && tab_index < tabs.size()) {
		if (tabs[tab_index].input_text) {
			tabs[tab_index].input_text->clear();
		}
	}
}

void UltimateAssistantPanel::_on_interrupt_pressed(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	_append_message(p_tab_id, TTR("System"), TTR("Interrupt requested (stub)."));
	tab.is_active = false;
	if (tab.send_button) {
		tab.send_button->set_text(TTR("Send"));
	}
	if (tab.steer_button) {
		tab.steer_button->set_visible(false);
	}
}

void UltimateAssistantPanel::_on_steer_pressed(int p_tab_id) {
	_append_message(p_tab_id, TTR("System"), TTR("Steer requested (stub)."));
}

void UltimateAssistantPanel::_on_settings_pressed() {
	settings_dialog->set_models(available_models);
	settings_dialog->popup_centered();
}

void UltimateAssistantPanel::_on_settings_confirmed() {
	PackedStringArray selected = settings_dialog->get_selected_models();
	if (selected.is_empty()) {
		return;
	}
	available_models = selected;
	_refresh_model_selectors();
}

void UltimateAssistantPanel::_on_tab_setting_changed(int p_tab_id) {
	_refresh_hub();
}

void UltimateAssistantPanel::_on_context_add_pressed(int p_tab_id) {
	_open_context_dialog(p_tab_id);
}

void UltimateAssistantPanel::_on_context_remove_pressed(int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	ItemList *list = tabs[tab_index].context_list;
	if (!list) {
		return;
	}
	PackedInt32Array selected = list->get_selected_items();
	for (int i = selected.size() - 1; i >= 0; i--) {
		list->remove_item(selected[i]);
	}
}

void UltimateAssistantPanel::_on_context_toggle_toggled(bool p_pressed, int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	tab.context_collapsed = !p_pressed;
	if (tab.context_list) {
		tab.context_list->set_visible(p_pressed);
	}
}

void UltimateAssistantPanel::_on_context_select_add_pressed() {
	if (!context_file_list || !context_selected_list) {
		return;
	}
	PackedInt32Array indices = context_file_list->get_selected_items();
	for (int i = 0; i < indices.size(); i++) {
		Variant item_meta = context_file_list->get_item_metadata(indices[i]);
		String value = item_meta.get_type() == Variant::STRING ? String(item_meta) : context_file_list->get_item_text(indices[i]);
		bool exists = false;
		int count = context_selected_list->get_item_count();
		for (int j = 0; j < count; j++) {
			Variant existing_meta = context_selected_list->get_item_metadata(j);
			String existing = existing_meta.get_type() == Variant::STRING ? String(existing_meta) : context_selected_list->get_item_text(j);
			if (existing == value) {
				exists = true;
				break;
			}
		}
		if (!exists) {
			int idx = context_selected_list->add_item(value);
			context_selected_list->set_item_metadata(idx, value);
		}
	}
}

void UltimateAssistantPanel::_on_context_select_remove_pressed() {
	if (!context_selected_list) {
		return;
	}
	PackedInt32Array indices = context_selected_list->get_selected_items();
	for (int i = indices.size() - 1; i >= 0; i--) {
		context_selected_list->remove_item(indices[i]);
	}
}

void UltimateAssistantPanel::_on_context_note_add_pressed() {
	if (!context_note_input || !context_selected_list) {
		return;
	}
	String value = context_note_input->get_text().strip_edges();
	if (value.is_empty()) {
		return;
	}
	int idx = context_selected_list->add_item(value);
	context_selected_list->set_item_metadata(idx, value);
	context_note_input->clear();
}

void UltimateAssistantPanel::_on_context_search_changed(const String &p_text) {
	_populate_context_file_list(p_text);
}

void UltimateAssistantPanel::_on_context_dialog_confirmed() {
	int tab_index = _find_tab_index_by_id(context_dialog_tab_id);
	if (tab_index < 0) {
		return;
	}
	ItemList *list = tabs[tab_index].context_list;
	if (!list || !context_selected_list) {
		return;
	}
	list->clear();
	int count = context_selected_list->get_item_count();
	for (int i = 0; i < count; i++) {
		String text = context_selected_list->get_item_text(i);
		Variant meta = context_selected_list->get_item_metadata(i);
		int idx = list->add_item(text);
		if (meta.get_type() != Variant::NIL) {
			list->set_item_metadata(idx, meta);
		}
	}
}

void UltimateAssistantPanel::_on_previous_reopen_pressed() {
	if (!hub_previous_list) {
		return;
	}
	PackedInt32Array selected = hub_previous_list->get_selected_items();
	if (selected.is_empty()) {
		return;
	}
	Variant meta = hub_previous_list->get_item_metadata(selected[0]);
	if (meta.get_type() != Variant::INT) {
		return;
	}
	int idx = meta;
	_restore_archived_session(idx);
}

void UltimateAssistantPanel::_on_previous_session_activated(int p_index) {
	if (!hub_previous_list) {
		return;
	}
	Variant meta = hub_previous_list->get_item_metadata(p_index);
	if (meta.get_type() != Variant::INT) {
		return;
	}
	int idx = meta;
	_restore_archived_session(idx);
}

void UltimateAssistantPanel::_on_session_name_changed(const String &p_text, int p_tab_id) {
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0 || tab_index >= tabs.size()) {
		return;
	}
	ChatTab &tab = tabs.write[tab_index];
	tab.display_name = p_text.strip_edges();
	int container_index = _find_tab_container_index(tab.root);
	if (container_index >= 0) {
		tab_container->set_tab_title(container_index, _get_tab_label(tab));
	}
	_refresh_hub();
}

void UltimateAssistantPanel::_open_context_dialog(int p_tab_id) {
	context_dialog_tab_id = p_tab_id;
	if (context_search_input) {
		context_search_input->clear();
	}
	if (context_note_input) {
		context_note_input->clear();
	}
	_sync_context_selected_list(p_tab_id);
	_populate_context_file_list(String());
	context_dialog->popup_centered();
}

void UltimateAssistantPanel::_populate_context_file_list(const String &p_filter) {
	if (!context_file_list) {
		return;
	}
	context_file_list->clear();
	Vector<String> files;
	EditorFileSystem *fs = EditorFileSystem::get_singleton();
	if (fs) {
		EditorFileSystemDirectory *root_dir = fs->get_filesystem();
		if (root_dir) {
			_collect_project_files(root_dir, files);
		}
	}
	String filter = p_filter.strip_edges();
	for (int i = 0; i < files.size(); i++) {
		if (!filter.is_empty() && files[i].findn(filter) == -1) {
			continue;
		}
		String type = EditorFileSystem::get_singleton() ? EditorFileSystem::get_singleton()->get_file_type(files[i]) : String();
		String label = type.is_empty() ? files[i] : vformat("%s  (%s)", files[i], type);
		Ref<Texture2D> icon = _get_file_icon_for_path(files[i]);
		int idx = context_file_list->add_item(label, icon);
		context_file_list->set_item_metadata(idx, files[i]);
	}
}

void UltimateAssistantPanel::_collect_project_files(EditorFileSystemDirectory *p_dir, Vector<String> &r_files) const {
	if (!p_dir) {
		return;
	}
	int file_count = p_dir->get_file_count();
	for (int i = 0; i < file_count; i++) {
		r_files.push_back(p_dir->get_file_path(i));
	}
	int dir_count = p_dir->get_subdir_count();
	for (int i = 0; i < dir_count; i++) {
		_collect_project_files(p_dir->get_subdir(i), r_files);
	}
}

void UltimateAssistantPanel::_sync_context_selected_list(int p_tab_id) {
	if (!context_selected_list) {
		return;
	}
	context_selected_list->clear();
	int tab_index = _find_tab_index_by_id(p_tab_id);
	if (tab_index < 0) {
		return;
	}
	ItemList *list = tabs[tab_index].context_list;
	if (!list) {
		return;
	}
	int count = list->get_item_count();
	for (int i = 0; i < count; i++) {
		String text = list->get_item_text(i);
		Variant meta = list->get_item_metadata(i);
		int idx = context_selected_list->add_item(text);
		if (meta.get_type() != Variant::NIL) {
			context_selected_list->set_item_metadata(idx, meta);
		}
	}
}

Ref<Texture2D> UltimateAssistantPanel::_get_file_icon_for_path(const String &p_path) const {
	EditorFileSystem *fs = EditorFileSystem::get_singleton();
	if (!fs) {
		return Ref<Texture2D>();
	}
	String dir_path = p_path.get_base_dir();
	EditorFileSystemDirectory *dir = fs->get_filesystem_path(dir_path);
	if (!dir) {
		return Ref<Texture2D>();
	}
	int idx = dir->find_file_index(p_path.get_file());
	if (idx < 0) {
		return Ref<Texture2D>();
	}
	String icon_path = dir->get_file_icon_path(idx);
	if (icon_path.is_empty()) {
		return Ref<Texture2D>();
	}
	Ref<Resource> res = ResourceLoader::load(icon_path);
	Ref<Texture2D> tex = res;
	return tex;
}

void UltimateAssistantPanel::_refresh_hub() {
	if (!hub_agent_list) {
		return;
	}
	hub_agent_list->clear();
	if (hub_previous_list) {
		hub_previous_list->clear();
	}
	if (hub_pending_list) {
		hub_pending_list->clear();
	}
	if (hub_questions_list) {
		hub_questions_list->clear();
	}
	for (int i = 0; i < tabs.size(); i++) {
		const ChatTab &tab = tabs[i];
		String model = TTR("Unknown");
		if (tab.model_selector && tab.model_selector->get_selected() >= 0) {
			model = tab.model_selector->get_item_text(tab.model_selector->get_selected());
		}
		String agent_mode = TTR("Local");
		if (tab.agent_mode_selector && tab.agent_mode_selector->get_selected() >= 0) {
			agent_mode = tab.agent_mode_selector->get_item_text(tab.agent_mode_selector->get_selected());
		}
		String status = TTR("Idle");
		String entry = vformat("%s  |  %s  |  %s  |  %s", _get_tab_label(tab), model, agent_mode, status);
		hub_agent_list->add_item(entry);

		if (hub_pending_list) {
			String pending = vformat("%s  |  Needs approval: edit sensitive file (stub)", _get_tab_label(tab));
			hub_pending_list->add_item(pending);
			String pending2 = vformat("%s  |  Run terminal command (stub)", _get_tab_label(tab));
			hub_pending_list->add_item(pending2);
		}
		if (hub_questions_list) {
			String question = vformat("%s  |  Question: Which scene should I update? (stub)", _get_tab_label(tab));
			hub_questions_list->add_item(question);
		}
	}
	if (hub_previous_list) {
		for (int i = 0; i < archived_sessions.size(); i++) {
			const ArchivedSession &archived = archived_sessions[i];
			String entry = vformat("%s  |  %s  |  %s", archived.display_title, archived.model, archived.agent_mode);
			int item_idx = hub_previous_list->add_item(entry);
			hub_previous_list->set_item_metadata(item_idx, i);
		}
	}
}

void UltimateAssistantPanel::_archive_tab(const ChatTab &p_tab) {
	ArchivedSession archived;
	archived.custom_name = p_tab.display_name.strip_edges();
	archived.display_title = _get_tab_label(p_tab);
	archived.model = TTR("Unknown");
	if (p_tab.model_selector && p_tab.model_selector->get_selected() >= 0) {
		archived.model = p_tab.model_selector->get_item_text(p_tab.model_selector->get_selected());
	}
	archived.agent_mode = TTR("Local");
	if (p_tab.agent_mode_selector && p_tab.agent_mode_selector->get_selected() >= 0) {
		archived.agent_mode = p_tab.agent_mode_selector->get_item_text(p_tab.agent_mode_selector->get_selected());
	}
	archived.transcript = p_tab.transcript;
	if (p_tab.context_list) {
		int count = p_tab.context_list->get_item_count();
		for (int i = 0; i < count; i++) {
			archived.context_items.push_back(p_tab.context_list->get_item_text(i));
			archived.context_metadata.push_back(p_tab.context_list->get_item_metadata(i));
		}
	}
	archived_sessions.push_back(archived);
}

void UltimateAssistantPanel::_restore_archived_session(int p_index) {
	if (p_index < 0 || p_index >= archived_sessions.size()) {
		return;
	}
	ArchivedSession archived = archived_sessions[p_index];
	archived_sessions.remove_at(p_index);

	int previous_count = tabs.size();
	_add_chat_tab();
	if (previous_count < 0 || previous_count >= tabs.size()) {
		_refresh_hub();
		return;
	}
	ChatTab &tab = tabs.write[previous_count];
	if (!archived.custom_name.is_empty()) {
		tab.display_name = archived.custom_name;
		if (tab.session_name_input) {
			tab.session_name_input->set_text(archived.custom_name);
		}
	}
	if (tab.chat_display) {
		if (!archived.transcript.is_empty()) {
			tab.chat_display->set_text(archived.transcript);
			tab.transcript = archived.transcript;
		}
	}
	if (tab.context_list && archived.context_items.size() > 0) {
		for (int i = 0; i < archived.context_items.size(); i++) {
			int idx = tab.context_list->add_item(archived.context_items[i]);
			Variant meta = archived.context_metadata.size() > i ? archived.context_metadata[i] : Variant();
			if (meta.get_type() != Variant::NIL) {
				tab.context_list->set_item_metadata(idx, meta);
			}
		}
	}
	int container_index = _find_tab_container_index(tab.root);
	if (container_index >= 0) {
		tab_container->set_tab_title(container_index, _get_tab_label(tab));
	}
	_refresh_hub();
}

String UltimateAssistantPanel::_get_tab_label(const ChatTab &p_tab) const {
	String name = p_tab.display_name.strip_edges();
	if (name.is_empty()) {
		return vformat("Agent %d", p_tab.id);
	}
	return name;
}
