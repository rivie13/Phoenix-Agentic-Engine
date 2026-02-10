/**************************************************************************/
/*  assistant_settings_dialog.cpp                                         */
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

#include "assistant_settings_dialog.h"

#include "core/object/class_db.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"

void UltimateAISettingsDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_add_pressed"), &UltimateAISettingsDialog::_on_add_pressed);
	ClassDB::bind_method(D_METHOD("_on_remove_pressed"), &UltimateAISettingsDialog::_on_remove_pressed);
}

UltimateAISettingsDialog::UltimateAISettingsDialog() {
	set_title(TTR("Assistant Settings"));
	set_ok_button_text(TTR("Apply"));
	set_min_size(Size2(460, 360));

	VBoxContainer *root = memnew(VBoxContainer);
	add_child(root);

	Label *models_label = memnew(Label);
	models_label->set_text(TTR("Available models"));
	root->add_child(models_label);

	model_list = memnew(ItemList);
	model_list->set_select_mode(ItemList::SELECT_MULTI);
	model_list->set_allow_reselect(true);
	model_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(model_list);

	HBoxContainer *add_row = memnew(HBoxContainer);
	root->add_child(add_row);

	model_input = memnew(LineEdit);
	model_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	model_input->set_placeholder(TTR("Add model id (ex: gpt-5.2-codex)"));
	add_row->add_child(model_input);

	add_button = memnew(Button);
	add_button->set_text(TTR("Add"));
	add_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAISettingsDialog::_on_add_pressed));
	add_row->add_child(add_button);

	remove_button = memnew(Button);
	remove_button->set_text(TTR("Remove Selected"));
	remove_button->connect(SceneStringName(pressed), callable_mp(this, &UltimateAISettingsDialog::_on_remove_pressed));
	root->add_child(remove_button);

	root->add_child(memnew(HSeparator));

	Label *service_label = memnew(Label);
	service_label->set_text(TTR("Service mode"));
	root->add_child(service_label);

	service_mode = memnew(OptionButton);
	service_mode->add_item(TTR("Managed (default)"));
	service_mode->add_item(TTR("BYOK"));
	service_mode->add_item(TTR("Local Hosting"));
	service_mode->select(0);
	root->add_child(service_mode);

	Label *managed_label = memnew(Label);
	managed_label->set_text(TTR("Managed endpoint"));
	root->add_child(managed_label);

	managed_endpoint = memnew(LineEdit);
	managed_endpoint->set_placeholder(TTR("https://api.phoenix.local"));
	root->add_child(managed_endpoint);

	Label *byok_label = memnew(Label);
	byok_label->set_text(TTR("BYOK API key"));
	root->add_child(byok_label);

	byok_key = memnew(LineEdit);
	byok_key->set_placeholder(TTR("sk-..."));
	byok_key->set_secret(true);
	root->add_child(byok_key);

	Label *local_label = memnew(Label);
	local_label->set_text(TTR("Local endpoint"));
	root->add_child(local_label);

	local_endpoint = memnew(LineEdit);
	local_endpoint->set_placeholder(TTR("http://localhost:11434"));
	root->add_child(local_endpoint);

	root->add_child(memnew(HSeparator));

	Label *rules_label = memnew(Label);
	rules_label->set_text(TTR("Rules"));
	root->add_child(rules_label);

	allow_background_agents = memnew(CheckBox);
	allow_background_agents->set_text(TTR("Allow background agents"));
	allow_background_agents->set_pressed(true);
	root->add_child(allow_background_agents);

	auto_approve_reads = memnew(CheckBox);
	auto_approve_reads->set_text(TTR("Auto-approve read tools"));
	auto_approve_reads->set_pressed(true);
	root->add_child(auto_approve_reads);

	require_approvals = memnew(CheckBox);
	require_approvals->set_text(TTR("Require approvals for write tools"));
	require_approvals->set_pressed(true);
	root->add_child(require_approvals);
}

void UltimateAISettingsDialog::set_models(const PackedStringArray &p_models) {
	model_list->clear();
	for (int i = 0; i < p_models.size(); i++) {
		model_list->add_item(p_models[i]);
		model_list->select(i, false);
	}
}

PackedStringArray UltimateAISettingsDialog::get_selected_models() const {
	PackedStringArray selected;
	PackedInt32Array indices = model_list->get_selected_items();
	for (int i = 0; i < indices.size(); i++) {
		int idx = indices[i];
		selected.push_back(model_list->get_item_text(idx));
	}
	return selected;
}

void UltimateAISettingsDialog::_on_add_pressed() {
	String new_model = model_input->get_text().strip_edges();
	if (new_model.is_empty()) {
		return;
	}
	int existing = -1;
	int item_count = model_list->get_item_count();
	for (int i = 0; i < item_count; i++) {
		if (model_list->get_item_text(i) == new_model) {
			existing = i;
			break;
		}
	}
	if (existing >= 0) {
		model_list->select(existing, true);
		model_input->clear();
		return;
	}

	int idx = model_list->add_item(new_model);
	model_list->select(idx, true);
	model_input->clear();
}

void UltimateAISettingsDialog::_on_remove_pressed() {
	PackedInt32Array indices = model_list->get_selected_items();
	for (int i = indices.size() - 1; i >= 0; i--) {
		model_list->remove_item(indices[i]);
	}
}
