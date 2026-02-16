/**************************************************************************/
/*  addon_bootstrap_utils.cpp                                             */
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

#include "addon_bootstrap_utils.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"

void AddonBootstrapMigrator::ensure_default_gitignore_entries_once() {
	static bool initialized = false;
	if (initialized) {
		return;
	}
	initialized = true;

	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (!project_settings) {
		return;
	}

	const String gitignore_path = project_settings->globalize_path("res://.gitignore");
	const bool has_existing = FileAccess::exists(gitignore_path);
	String gitignore_contents;

	if (has_existing) {
		Ref<FileAccess> file = FileAccess::open(gitignore_path, FileAccess::READ);
		if (file.is_null()) {
			return;
		}
		gitignore_contents = file->get_as_text();
	}

	bool gitignore_changed = false;
	if (!has_existing || gitignore_contents.is_empty()) {
		gitignore_contents = "# Godot 4+ specific ignores\n.godot/\n";
		gitignore_changed = true;
	} else if (gitignore_contents.find(".godot/") == -1) {
		if (!gitignore_contents.ends_with("\n")) {
			gitignore_contents += "\n";
		}
		gitignore_contents += ".godot/\n";
		gitignore_changed = true;
	}

	if (!gitignore_changed) {
		return;
	}

	Ref<FileAccess> out = FileAccess::open(gitignore_path, FileAccess::WRITE);
	if (out.is_null()) {
		return;
	}
	out->store_string(gitignore_contents);
}
