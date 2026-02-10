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

#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "core/variant/variant.h"
#include "scene/gui/panel_container.h"

class Button;
class Control;
class HBoxContainer;
class Label;
class LineEdit;
class OptionButton;
class RichTextLabel;
class ScrollContainer;
class TabContainer;
class VBoxContainer;
class ItemList;
class CheckBox;
class AcceptDialog;
class TextEdit;
class VSplitContainer;
class EditorFileSystemDirectory;
class Texture2D;

class UltimateAISettingsDialog;

class UltimateAssistantPanel : public PanelContainer {
	GDCLASS(UltimateAssistantPanel, PanelContainer);

	struct ChatTab {
		int id = 0;
		String display_name;
		String transcript;
		VBoxContainer *root = nullptr;
		OptionButton *mode_selector = nullptr;
		OptionButton *model_selector = nullptr;
		OptionButton *agent_mode_selector = nullptr;
		LineEdit *session_name_input = nullptr;
		ItemList *context_list = nullptr;
		VBoxContainer *context_section = nullptr;
		Button *context_toggle_button = nullptr;
		bool context_collapsed = false;
		Button *context_add_button = nullptr;
		Button *context_remove_button = nullptr;
		RichTextLabel *chat_display = nullptr;
		TextEdit *input_text = nullptr;
		Button *send_button = nullptr;
		Button *steer_button = nullptr;
		bool is_active = false;
	};

	struct ArchivedSession {
		String custom_name;
		String display_title;
		String model;
		String agent_mode;
		String transcript;
		PackedStringArray context_items;
		Array context_metadata;
	};

	struct SharedChatTabState {
		int id = 0;
		String display_name;
		String transcript;
		int mode_selected = 0;
		int model_selected = 0;
		int agent_mode_selected = 0;
		String session_name_text;
		PackedStringArray context_items;
		Array context_metadata;
		bool context_collapsed = false;
		bool is_active = false;
	};

	VBoxContainer *root = nullptr;
	HBoxContainer *header = nullptr;
	Label *title_label = nullptr;
	Button *new_tab_button = nullptr;
	Button *settings_button = nullptr;
	TabContainer *tab_container = nullptr;
	UltimateAISettingsDialog *settings_dialog = nullptr;
	AcceptDialog *context_dialog = nullptr;
	LineEdit *context_search_input = nullptr;
	LineEdit *context_note_input = nullptr;
	ItemList *context_file_list = nullptr;
	ItemList *context_selected_list = nullptr;
	Button *context_select_add_button = nullptr;
	Button *context_select_remove_button = nullptr;
	Button *context_note_add_button = nullptr;
	int context_dialog_tab_id = -1;
	VBoxContainer *hub_root = nullptr;
	ItemList *hub_agent_list = nullptr;
	ItemList *hub_previous_list = nullptr;
	Button *hub_reopen_button = nullptr;
	ItemList *hub_pending_list = nullptr;
	ItemList *hub_questions_list = nullptr;

	Vector<ChatTab> tabs;
	Vector<ArchivedSession> archived_sessions;
	PackedStringArray available_models;
	int tab_counter = 0;
	bool theme_ready = false;
	bool applying_shared_state = false;

	static Vector<UltimateAssistantPanel *> s_instances;
	static Vector<SharedChatTabState> s_shared_tabs;
	static Vector<ArchivedSession> s_shared_archived_sessions;
	static PackedStringArray s_shared_models;
	static Dictionary s_shared_pixelpen_snapshot;
	static Array s_shared_pixelpen_layers;
	static int s_shared_tab_counter;
	static int s_shared_current_tab;
	static bool s_shared_initialized;

	void _on_new_tab_pressed();
	void _on_tab_close_requested(int p_tab_index);
	void _on_send_pressed(int p_tab_id);
	void _on_input_submitted(const String &p_text, int p_tab_id);
	void _on_interrupt_pressed(int p_tab_id);
	void _on_steer_pressed(int p_tab_id);
	void _on_settings_pressed();
	void _on_settings_confirmed();
	void _on_tab_setting_changed(int p_tab_id);
	void _on_context_add_pressed(int p_tab_id);
	void _on_context_remove_pressed(int p_tab_id);
	void _on_context_select_add_pressed();
	void _on_context_select_remove_pressed();
	void _on_context_note_add_pressed();
	void _on_context_search_changed(const String &p_text);
	void _on_context_toggle_toggled(bool p_pressed, int p_tab_id);
	void _on_context_dialog_confirmed();
	void _on_previous_reopen_pressed();
	void _on_previous_session_activated(int p_index);
	void _on_session_name_changed(const String &p_text, int p_tab_id);
	void _append_message(int p_tab_id, const String &p_role, const String &p_content);
	void _add_chat_tab(int p_forced_id = -1);
	void _add_chat_tab_from_state(const SharedChatTabState &p_state);
	void _refresh_model_selectors();
	void _ensure_default_models();
	int _find_tab_index_by_id(int p_tab_id) const;
	int _find_tab_index_by_root(Control *p_root) const;
	int _find_tab_container_index(Control *p_root) const;
	void _open_context_dialog(int p_tab_id);
	void _populate_context_file_list(const String &p_filter);
	void _collect_project_files(EditorFileSystemDirectory *p_dir, Vector<String> &r_files) const;
	void _sync_context_selected_list(int p_tab_id);
	Ref<Texture2D> _get_file_icon_for_path(const String &p_path) const;
	void _build_hub_tab();
	void _refresh_hub();
	void _refresh_tab_close_icons();
	void _archive_tab(const ChatTab &p_tab);
	void _restore_archived_session(int p_index);
	String _get_tab_label(const ChatTab &p_tab) const;
	SharedChatTabState _build_shared_state_for_tab(const ChatTab &p_tab) const;
	void _capture_shared_state();
	void _apply_shared_state();
	void _broadcast_shared_state();
	void _clear_tabs_ui();
	void _sync_pixelpen_snapshot_to_tabs();
	void _upsert_pixelpen_snapshot(ChatTab &r_tab);
	void _populate_pixelpen_context_items(const String &p_filter);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	UltimateAssistantPanel();
	static void broadcast_pixelpen_context(const Dictionary &p_snapshot, const Array &p_layers);
};
