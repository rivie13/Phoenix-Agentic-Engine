# Core Modifications Tracker

This file tracks intentional Phoenix-specific changes that are expected to diverge from upstream.

## Files Modified Outside `modules/ultimate_ai`

### `misc/scripts/build_godot_git_plugin.sh`

- Patch the cloned git-plugin build to disable libgit2 Clar tests and apply macOS-specific fixes for clang warnings and zlib `fdopen` macro conflicts.
- On macOS, patch cloned `tools/git2.py` to link `iconv` when building the plugin shared library.
- Why: macOS universal builds were failing under newer Xcode due to zlib macro conflicts and libgit2 test links pulling the wrong-arch Homebrew `libssh2`.
- Why: static `libgit2.a` can reference iconv symbols that are not auto-propagated to the final plugin link, causing `_iconv*` undefined symbol failures.
- Merge note: keep macOS guards and test-disable patches unless upstream libgit2/cmake options are updated to avoid these issues.

### `misc/scripts/check_ci_log.py`

- Added `GODOT_CI_ALLOW_OBJECTDB_LEAKS` override to skip failing CI when ObjectDB leak warnings are expected.
- Why: the GCC sanitizer project test currently reports ObjectDB leaks during shutdown; we still want other fatal errors to fail the job.
- Merge note: remove the override once the underlying leak is fixed.

### `.github/workflows/linux_builds.yml`

- Set `GODOT_CI_ALLOW_OBJECTDB_LEAKS=1` for the GCC sanitizer matrix job only.
- Why: keep project tests running while temporarily ignoring the known ObjectDB leak warning in that configuration.
- Merge note: drop the flag when the leak is resolved.
- Build and stage editor addons for Linux editor artifacts:
  - Build PixelPen bindings on editor artifact jobs.
  - Stage `net.yarvis.pixel_pen` and `diff-margin` into `bin/addons` before upload.
- Why: Linux downloadable editor artifacts should include the Phoenix addon set by default.
- Merge note: keep addon staging in sync with Windows workflow policy.
- Added pre-upload artifact guard checks for expected binary path and required addon directories on editor artifacts.
- Why: fail fast when naming/staging regressions occur before publishing broken artifacts.
- Merge note: keep guards aligned with artifact naming and staged addon policy.

### `misc/scripts/validate_extension_api.sh`

- Normalized validation output to treat `ERROR: Validate extension JSON:` the same as `Validate extension JSON:`.
- Why: Godot's logger prefixes some validation lines with `ERROR:`, which made the compatibility checker ignore real incompatibilities and pass while printing errors.
- Merge note: keep normalization unless upstream standardizes the output prefix again.

### `core/extension/extension_api_dump.cpp`

- Emit `arguments` only when non-empty for utility functions, builtin methods/constructors, class methods, and signals in `extension_api.json`.
- Why: avoid false GDExtension compatibility failures against stable reference APIs that omit empty `arguments` arrays.
- Merge note: keep the dump format aligned with upstream compatibility validation expectations.

### `.github/workflows/macos_builds.yml`

- Made Xcode selection resilient (falls back to `/Applications/Xcode.app` when the pinned path isn't present).
- Installed dependencies needed by `build_godot_git_plugin.sh` (cmake/perl/nasm) on editor builds.
- Why: macOS CI can fail if a specific Xcode isn't installed or if required tools aren't present.
- Merge note: prefer upstream CI approach if they standardize Xcode selection and dependency installation.
- Build and stage editor addons for macOS editor artifacts:
  - Build PixelPen bindings from dumped extension API.
  - Stage `net.yarvis.pixel_pen` and `diff-margin` into `bin/addons` before upload.
- Why: macOS downloadable editor artifacts should include the Phoenix addon set by default.
- Merge note: keep addon staging in sync with Linux/Windows workflow policy.
- Added pre-upload artifact guard checks for expected universal binary path and required addon directories on editor artifacts.
- Why: fail fast when naming/staging regressions occur before publishing broken artifacts.
- Merge note: keep guards aligned with artifact naming and staged addon policy.

### `.github/workflows/windows_builds.yml`

- Added diff-margin addon staging into `bin/addons/diff-margin` for MSVC editor artifact builds.
- Added pre-upload artifact guard checks for expected binary path and required addon directories on editor artifacts.
- Why: align Windows packaged editor content with Linux/macOS and fail fast on packaging regressions.
- Merge note: keep guards and addon staging aligned with cross-platform artifact policy.

### `.github/workflows/android_builds.yml`

- Added pre-upload artifact guard checks for:
  - template jobs: expected Android template outputs in `bin`
  - editor jobs: expected `bin/android_editor_builds` and non-empty `horizonos`/`picoos` split artifact folders
- Why: catch missing Android artifacts before upload steps.
- Merge note: adjust guard patterns if Gradle output layout changes upstream.

### `.github/workflows/ios_builds.yml`

- Added pre-upload artifact guard check to verify iOS outputs exist in `bin`.
- Why: catch missing iOS template artifacts before upload.
- Merge note: adjust glob pattern if iOS output naming changes upstream.

### `.github/workflows/web_builds.yml`

- Added pre-upload artifact guard check to verify expected Web outputs (`.wasm`, `.js`, `.zip`) exist in `bin`.
- Why: catch missing Web template artifacts before upload.
- Merge note: adjust patterns if Web output naming/layout changes upstream.

### `.github/workflows/build.yml`

- Added diff-margin addon staging into `bin/addons/diff-margin` in the Windows aggregate build workflow.
- Added pre-upload artifact guard checks for expected editor binary and required addon directories.
- Why: ensure aggregate Windows build uploads include complete Phoenix addon payload and fail fast on packaging regressions.
- Merge note: keep guard expectations in sync with Windows artifact/addon policy.

### `platform/linuxbsd/SCsub`

- Changed Linux binary output basename from `godot` to `version.short_name` (Phoenix currently `phoenix_agentic`).
- Why: align produced Linux binaries with Phoenix naming used by CI artifact paths and release packaging.
- Merge note: if upstream keeps `godot` naming, preserve Phoenix fork naming for release consistency.

### `platform/macos/SCsub`

- Changed macOS binary output basename from `godot` to `version.short_name` (Phoenix currently `phoenix_agentic`).
- Why: align produced macOS binaries with Phoenix naming used by CI artifact paths and release packaging.
- Merge note: if upstream keeps `godot` naming, preserve Phoenix fork naming for release consistency.

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

### `drivers/metal/pixel_formats.cpp`

- Whitespace-only: `clang-format` realigned macro continuation backslashes (`\`) in `addDataFormatDescFull`, `addMTLPixelFormatDescFull`, `addMTLPixelFormatDescSRGB`, and `addMTLVertexFormatDesc` macro definitions.
- Why: pre-commit `clang-format` hook reformatted these lines automatically; no logic changes.
- Merge note: trivially resolved — accept either side during upstream merges.

### `drivers/metal/rendering_device_driver_metal.cpp`

- Whitespace-only: `clang-format` realigned macro continuation backslashes in `ADD_USAGE` and `UNKNOWN` macro definitions.
- Why: pre-commit `clang-format` hook reformatted these lines automatically; no logic changes.
- Merge note: trivially resolved — accept either side during upstream merges.

### `servers/rendering/renderer_rd/effects/SCsub`

- Avoid building both `metal_fx.cpp` and `metal_fx.mm` when Metal is enabled by filtering the cpp list.
- Why: iOS Metal builds were producing duplicate object targets for `metal_fx`, failing SCons with “Multiple ways to build the same target”.
- Merge note: keep the conditional filter or align with upstream if they change MetalFX build selection.

### `doc/tools/make_rst.py`

- Skip walking into `modules/**/external` when collecting `doc_classes` XML files.
- Why: avoid duplicate class definitions from third-party submodules (e.g., nested `godot-cpp` tests) that break `make-rst` in pre-commit.
- Merge note: keep exclusion or upstream-ignore rules if external submodules are used there too.

### `drivers/metal/SCsub`

- Avoid building both `.cpp` and `.mm` siblings by filtering out any `.cpp` with a matching `.mm` basename.
- Why: iOS Metal builds were producing duplicate object targets (e.g., `metal_device_properties`, `pixel_formats`), failing SCons with “Multiple ways to build the same target”.
- Merge note: keep the conditional filter or align with upstream if they change the driver file selection.

### `editor/version_control/version_control_editor_plugin.h`

- Added `ensure_vcs_plugin_loaded(...)` public helper.
- Why: allow Phoenix editor plugins to auto-load the VCS UI once a VCS plugin is available.
- Merge note: if upstream adds a similar hook, prefer upstream API and drop this helper.
- Added per-file VCS action button ids for diff/unstage/gitignore.
- Why: support new per-file actions in the VCS dock UI.
- Merge note: align with upstream button ids if they introduce equivalent actions.

### `editor/version_control/version_control_editor_plugin.cpp`

- Implemented `ensure_vcs_plugin_loaded(...)` to load a named VCS plugin and persist autoload settings.
- Why: auto-connect the Git VCS interface and show the standard Version Control docks without manual setup.
- Merge note: reapply or replace with upstream autoload mechanism if introduced.
- Set the Version Control commit dock default slot to `DOCK_SLOT_LEFT_BR` and auto-open it after registration.
- Why: keep the Git UI next to the FileSystem dock and visible by default.
- Merge note: adjust to upstream docking defaults if they add configurable placement.
- Block in-editor branch checkout when Phoenix worktree mode is required.
- Why: Phoenix workflow mandates separate worktrees per branch to avoid stale editor caches and cross-branch asset state.
- Merge note: keep or replace with upstream worktree-safe branch switching if added.
- Added a Phoenix worktree switch dialog that creates/opens worktrees and relaunches the editor.
- Why: enable low-friction branch viewing while keeping each branch isolated in its own worktree.
- Merge note: keep or replace with upstream worktree UI if added.
- Default worktree root now points to `user://.phoenix_worktrees` and auto-adds in-repo roots to `.gitignore`.
- Why: keep worktree folders out of Git status by default while still allowing per-project overrides.
- Merge note: preserve this behavior unless upstream provides a first-class worktree manager.
- Added per-file VCS list buttons for diff, unstage, and add-to-gitignore actions.
- Why: expose common per-file actions directly in the VCS dock without extra steps.
- Merge note: reapply if upstream adds similar per-file actions in the VCS UI.

## `modules/ultimate_ai` Integration Changes

### Submodule wiring

- `.gitmodules` now includes:
  - `modules/ultimate_ai/external/pixelpen` -> `https://github.com/rivie13/pixelpen.git`
  - `modules/ultimate_ai/external/godot-diff-margin` -> `https://github.com/rivie13/godot-diff-margin.git`
  - `modules/ultimate_ai/external/godot-git-plugin` -> `https://github.com/rivie13/godot-git-plugin.git`
- Nested submodule under PixelPen:
  - `modules/ultimate_ai/external/pixelpen/godot-cpp`

### Editor plugin registration

- `modules/ultimate_ai/register_types.cpp`
  - Registers `UltimateAIEditorPlugin`
  - Registers `DiffMarginEditorPlugin`
  - Registers `GitPluginEditorPlugin`
  - Registers `PixelPenEditorPlugin`
  - Registers `GitPluginEditorPlugin`

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
  - revision value: `2026-02-10-pixelpen-addon-preload-order-fix`
- Behavior:
  - If addon + marker match current revision, skip recopy.
  - If marker missing or revision mismatch, recopy addon from submodule source and rescan filesystem.
  - After recopy, defer opening PixelPen until the filesystem scan completes.
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

- Ensured editor class registration for tool scripts:
  - `classes/pixelpen_enum.gd` (`@tool`)
  - `classes/mask_selection.gd` (`@tool`)

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

## Diff Margin Addon Compatibility Patches (Submodule)

The following file was patched under:
`modules/ultimate_ai/external/godot-diff-margin/addons/diff-margin`

- `plugin.gd`
  - Use `EditorSettings.set_setting(...)` API (was `set_settings`)
  - Fix `_diffs_map` declaration syntax (`:=`)
  - Treat empty string metadata as unset for gutter rendering
  - Skip gutter setup when the current editor is not a script or when viewing Phoenix diff files

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
