---
excludeAgent: "coding-agent"
---

# Copilot code review instructions (Engine repo)

You are reviewing PRs for a Godot fork with Phoenix editor integrations.
Priorities: minimal diffs, low upstream-merge risk, deterministic build/package behavior.

## Review focus

- Prefer changes in `modules/ultimate_ai/` and addon build scripts (`misc/scripts/build_*`, workflow addon staging).
- Treat edits under `core/`, `scene/`, `servers/`, `editor/`, `platform/`, `drivers/` as high risk.
- Require `modules/ultimate_ai/CORE_MODIFICATIONS.md` updates for Phoenix-driven edits outside `modules/ultimate_ai/`.

## Current integration surface to protect

- Module init (`modules/ultimate_ai/register_types.cpp`) should keep editor-only registration coherent:
  - `UltimateAIEditorPlugin`, `BfxrEditorPlugin`, `DiffMarginEditorPlugin`, `GDTermEditorPlugin`, `GitPluginEditorPlugin`, `GutEditorPlugin`, `PixelPenEditorPlugin`
  - `BfxrRuntimeBridge`, `UltimateAIBackendContractAdapter`, `UltimateAITerminalBridge`
- Addon payload staging must remain valid for CI/editor artifacts (`bin/addons/*`, bundled Node runtime for BFXR).

## Safety / correctness checks

- Enforce Godot style and memory/ownership safety (`memnew`/`memdelete`, `Ref<T>`, no exceptions).
- Verify editor-only logic remains under editor guards and editor-only init level assumptions.
- Flag regressions in addon bootstrap paths, sync markers, and auto-enable behavior.

## Scope hygiene

- Keep PRs narrow; split unrelated refactors.
- Avoid direct edits inside vendored third-party trees under `modules/ultimate_ai/external/*` unless intentionally updating submodule pointers.
- Reject prompt templates, secrets, credentials, or backend-only proprietary logic in public client code.
