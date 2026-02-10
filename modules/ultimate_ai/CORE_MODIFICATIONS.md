# Core Modifications Tracker

This file tracks intentional Phoenix-specific changes that are expected to diverge from upstream.

## Files Modified Outside `modules/ultimate_ai`

### `editor/icons/DefaultProjectIcon.svg`

- Replaced upstream icon SVG (PNG embedded in `<image>`) with vector paths from `branding/Phoenix_app_icon.svg`.
- Why: ThorVG does not reliably render PNG data URIs in this SVG, causing:
  - `ERROR: Invalid image: image is empty`
  - `ERROR: Image width must be greater than 0.`
- Merge note: keep Phoenix version of this icon during upstream merges.

### `editor/inspector/editor_resource_preview.cpp`

- Relaxed `EditorResourcePreview::get_preview_metadata()` to return an empty `Dictionary` when cache entries are not yet available.
- Why: avoids log spam when tooltips request preview metadata before the preview cache is populated.
- Merge note: safe behavior change; re-evaluate if upstream changes how tooltip previews are fetched.

No additional core engine files outside `modules/ultimate_ai` were modified as part of the PixelPen integration work.

## `modules/ultimate_ai` Integration Changes

### Submodule wiring

- `.gitmodules` now includes:
  - `modules/ultimate_ai/external/pixelpen` -> `https://github.com/rivie13/pixelpen.git`
- Nested submodule under PixelPen:
  - `modules/ultimate_ai/external/pixelpen/godot-cpp`

### Editor plugin registration

- `modules/ultimate_ai/register_types.cpp`
  - Registers `UltimateAIEditorPlugin`
  - Registers `PixelPenEditorPlugin`

No edits were required in `editor/editor_node.cpp` for this module plugin path.

### PixelPen editor integration

- Added:
  - `modules/ultimate_ai/ui/pixelpen_editor_plugin.h`
  - `modules/ultimate_ai/ui/pixelpen_editor_plugin.cpp`

Current behavior:

- PixelPen appears as a main nav screen via `has_main_screen()`.
- Selecting PixelPen opens PixelPen in a separate window.
- Previous main screen is restored after launch.
- Tool menu item exists: `PixelPen: Open Window`.
- PixelPen window is composed as a split view with PixelPen UI + assistant panel.
- PixelPen context is polled and forwarded into assistant shared state.

### Addon sync and rollout policy

- `modules/ultimate_ai/ui/pixelpen_editor_plugin.cpp` now writes a sync marker inside the copied project addon:
  - marker file: `res://addons/net.yarvis.pixel_pen/.phoenix_sync_revision`
  - revision value: `2026-02-09-pixelpen-addon-classname-fix`
- Behavior:
  - If addon + marker match current revision, skip recopy.
  - If marker missing or revision mismatch, recopy addon from submodule source and rescan filesystem.
- Why:
  - Ensures script compatibility fixes actually roll out to existing projects.
  - Avoids recopying addon every startup once project addon is in sync.

### Assistant sync and PixelPen context bridge

- Updated:
  - `modules/ultimate_ai/ui/assistant_panel.h`
  - `modules/ultimate_ai/ui/assistant_panel.cpp`

Current behavior:

- Assistant panel state is shared across instances (main dock and PixelPen window).
- Tabs, transcripts, model/mode selections, and context are synchronized.
- PixelPen snapshot and layer metadata are available in context picker.
- PixelPen context updates are broadcast to all assistant instances.

## PixelPen Addon Compatibility Patches (Submodule)

The following files were patched under:
`modules/ultimate_ai/external/pixelpen/project/addons/net.yarvis.pixel_pen`

### Class dependency stabilization for Godot 4.7 startup order

- Added explicit preloads to avoid global `class_name` registration race failures:
  - `classes/pixelpen.gd`
  - `classes/pixelpen_state.gd`
  - `classes/project_packer.gd`
  - `classes/animation_cell.gd`
  - `classes/frame.gd`
  - `classes/indexed_color_image.gd`
  - `classes/theme_config.gd`
  - `classes/user_config.gd`
  - `ui/layout_split/data_branch.gd`
  - `ui/layout_split/layout_split.gd`
  - `pixelpen_plugin.gd`

### Parse-order cycle reductions

- Replaced direct `PixelPen` singleton references in hot class scripts with runtime-loaded helper accessors (`_pixelpen()`) where needed:
  - `classes/pixel_pen_project.gd`
  - `classes/theme_config.gd`
  - `classes/user_config.gd`
  - `classes/frame.gd`
  - `classes/indexed_color_image.gd`

### Static self-reference cleanup

- `classes/mask_selection.gd`
  - Replaced `MaskSelection.create_image(...)` static self-calls with local `create_image(...)`.
- `ui/layout_split/branch.gd`
  - Updated `create(...)` to instantiate via script load instead of global class-name constructor.

### Why these addon patches were required

- Without these changes, project startup produced large parse cascades such as:
  - `Could not find type "PixelPenProject" in the current scope`
  - `Identifier "PixelPen" not declared in the current scope`
  - `Could not find type "DataBranch" in the current scope`

## Build and Runtime Validation Notes

Validated in this repo state:

- PixelPen nested `godot-cpp` initialized and pinned (`863d732` at validation time).
- PixelPen debug/release extension binaries built in:
  - `modules/ultimate_ai/external/pixelpen/project/addons/net.yarvis.pixel_pen/bin`
- Phoenix editor build succeeded:
  - `python -m SCons platform=windows target=editor d3d12=no`
- Headless project startup check succeeded for:
  - `C:/Users/rivie/PhoenixAgenticEngineProjects/asdfjjjj`
- Filtered startup checks no longer reported PixelPen parse/load errors.

## Merge Guidance

- Keep Phoenix icon replacement unless upstream fixes SVG rendering behavior for this asset.
- Keep `PixelPenEditorPlugin` registration in `modules/ultimate_ai/register_types.cpp`.
- Keep `.gitmodules` PixelPen entry.
- Keep addon sync marker logic in `modules/ultimate_ai/ui/pixelpen_editor_plugin.cpp`.
- Treat PixelPen addon compatibility patches in the PixelPen submodule as required for stable 4.7 startup parsing.
