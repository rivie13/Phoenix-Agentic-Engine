---
applyTo: "**"
excludeAgent: "coding-agent"
---

# Copilot code review instructions (Engine repo)

You are reviewing PRs for a fork of the Godot Engine.
Priorities: keep diffs minimal, avoid upstream-merge conflicts, and follow Godot conventions.

## Review focus

- Prefer changes inside `modules/ultimate_ai/` (and editor plugin code under `editor/plugins/`) over touching Godot core.
- Treat edits under `core/`, `scene/`, `servers/`, `editor/`, `platform/`, `drivers/` as **high-risk** (merge conflicts, regressions). If touched, ask for a clear justification.
- If any code outside `modules/ultimate_ai/` changes due to Phoenix features, require an entry in `modules/ultimate_ai/CORE_MODIFICATIONS.md` describing what changed and why it could not live in the module.

## Godot coding standards (what to enforce)

- Match existing style in the edited file (tabs vs spaces, brace style, include order).
- C++: use Godot conventions (snake_case for funcs/vars, PascalCase for classes); prefer Godot core types (`String`, `Array`, `Dictionary`, etc.) over STL types unless the surrounding code uses STL.
- Use Godot error handling patterns (`Error`, `ERR_FAIL_*`, `ERR_*_MSG`) instead of exceptions.
- Avoid introducing new global state or tight coupling; prefer signals/callbacks patterns used in nearby code.

## Safety / correctness checks

- Flag any threading or lifetime hazards (dangling pointers, ownership confusion, RefCounted misuse, `memnew`/`memdelete` mismatches).
- Verify editor-only code stays editor-only (guarded by `TOOLS_ENABLED` patterns where applicable) and doesn’t leak into export templates.
- Watch for accidental performance regressions in hot paths (per-frame loops, allocations in `_process`, unnecessary string conversions).

## Scope & hygiene

- Keep PR scope tight; ask to split unrelated refactors.
- Avoid vendoring large third‑party code or assets without license/attribution.
- Avoid adding secrets, credentials, or proprietary prompt content anywhere in the repo.

## What to request from authors (when missing)

- Minimal repro / validation notes (what they built, what they ran, platform tested).
- If build steps are relevant: standard Godot SCons workflow (e.g., `python -m SCons platform=<platform> target=editor`).
- If tests are added/changed: ensure they are deterministic and don’t require network access.
