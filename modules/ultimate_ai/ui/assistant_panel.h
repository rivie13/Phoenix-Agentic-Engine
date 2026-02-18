/**************************************************************************/
/*  assistant_panel.h                                                     */
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
class WebSocketPeer;

class UltimateAISettingsDialog;
class UltimateAIBackendContractAdapter;
class UltimateAIFrontendRuntimeAdapter;

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
		Label *status_label = nullptr;
		Button *send_button = nullptr;
		Button *copy_button = nullptr;
		Button *steer_button = nullptr;
		Button *resync_button = nullptr;
		VBoxContainer *approval_section = nullptr;
		Label *approval_label = nullptr;
		ItemList *approval_list = nullptr;
		Button *approve_button = nullptr;
		Button *reject_button = nullptr;
		bool is_active = false;
		bool has_conflict = false;
		bool realtime_bootstrapped = false;
		String session_id;
		String idempotency_key;
		String realtime_user_id;
		String realtime_url;
		String realtime_access_token;
		Ref<WebSocketPeer> realtime_peer;
		bool realtime_stream_failed = false;
		String last_plan_id;
		bool status_poll_active = false;
		uint64_t next_status_poll_msec = 0;
		bool loading_indicator_active = false;
		String loading_indicator_text;
		uint64_t next_loading_tick_msec = 0;
		int loading_indicator_phase = 0;
		bool loading_chat_notice_emitted = false;
		bool loading_chat_indicator_active = false;
		int loading_chat_indicator_prefix_len = 0;
		bool assistant_stream_open = false;
		String assistant_stream_text;
		int assistant_stream_prefix_len = 0;
		bool thinking_stream_open = false;
		String thinking_stream_text;
		int thinking_stream_prefix_len = 0;
		bool command_stream_active = false;
		String command_stream_role;
		String command_stream_remaining;
		uint64_t next_command_stream_tick_msec = 0;
		String last_reported_task_status;
		int last_realtime_seq = -1;
		bool suppress_next_chat_message = false;
		PackedStringArray pending_action_ids;
		Array pending_actions;
		int task_counter = 0;
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
	UltimateAIBackendContractAdapter *backend_adapter = nullptr;
	UltimateAIFrontendRuntimeAdapter *frontend_runtime_adapter = nullptr;
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
	void _on_copy_pressed(int p_tab_id);
	void _on_input_submitted(const String &p_text, int p_tab_id);
	void _on_interrupt_pressed(int p_tab_id);
	void _on_steer_pressed(int p_tab_id);
	void _on_settings_pressed();
	void _on_settings_confirmed();
	void _on_tab_setting_changed(int p_selected_index, int p_tab_id);
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
	void _on_approve_pressed(int p_tab_id);
	void _on_reject_pressed(int p_tab_id);
	void _on_resync_pressed(int p_tab_id);
	void _set_tab_busy(int p_tab_id, bool p_busy, const String &p_status = String());
	void _set_tab_status(int p_tab_id, const String &p_status, bool p_error = false);
	void _set_tab_loading_state(int p_tab_id, bool p_active, const String &p_base_text = String());
	void _update_loading_chat_indicator_for_tab(int p_tab_id);
	void _clear_loading_chat_indicator_for_tab(int p_tab_id);
	void _append_task_status_message(int p_tab_id, const String &p_status, bool p_force = false);
	void _start_command_stream_for_tab(int p_tab_id, const String &p_role, const String &p_content);
	Dictionary _build_project_map_payload(int p_tab_id) const;
	Dictionary _build_task_context_payload(int p_tab_id) const;
	String _resolve_runtime_user_id() const;
	bool _start_session_for_tab(int p_tab_id, bool p_force_resync = false);
	bool _bootstrap_realtime_for_tab(int p_tab_id);
	bool _connect_realtime_stream_for_tab(int p_tab_id);
	void _disconnect_realtime_stream_for_tab(int p_tab_id);
	void _poll_realtime_stream_for_tab(int p_tab_id);
	void _request_task_for_tab_deferred(int p_tab_id, const String &p_user_input);
	void _request_task_for_tab(int p_tab_id, const String &p_user_input);
	void _apply_realtime_events_for_tab(int p_tab_id, const Array &p_events, bool p_refresh_status = true);
	void _append_stream_delta_for_tab(int p_tab_id, const String &p_role, const String &p_delta, bool p_finish);
	void _append_thinking_delta_for_tab(int p_tab_id, const String &p_delta, bool p_finish);
	void _append_lock_snapshot_for_tab(int p_tab_id, const String &p_reason = String());
	void _apply_conflict_state(int p_tab_id, const Dictionary &p_response);
	void _clear_conflict_state(int p_tab_id);
	void _show_approval_batch(int p_tab_id, const Dictionary &p_batch);
	void _clear_approval_batch(int p_tab_id);
	bool _poll_task_status_for_tab(int p_tab_id, const String &p_plan_id);
	void _submit_approval_for_tab(int p_tab_id, const String &p_decision);
	void _execute_commands_for_tab(int p_tab_id, const Array &p_commands);
	void _execute_single_command(int p_tab_id, const Dictionary &p_command);
	void _log_request_context(const String &p_operation, const Dictionary &p_response) const;
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
