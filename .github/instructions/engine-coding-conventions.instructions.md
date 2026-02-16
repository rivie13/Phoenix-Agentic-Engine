# Coding conventions — Phoenix Agentic Engine (Godot fork)

## General rule

Match the style of the file you are editing (indentation, naming, include order, comment style).

## C++ conventions (Godot style)

- **Indentation**: tabs (not spaces).
- **Naming**: `snake_case` for functions/variables, `PascalCase` for classes.
- **Types**: prefer Godot core types (`String`, `Array`, `Dictionary`, `Variant`) over STL where practical.
- **Error handling**: use `Error` and `ERR_FAIL_*`/`ERR_*_MSG`; do not introduce exceptions.
- **Memory/ownership**: use `memnew`/`memdelete` and `Ref<T>` patterns consistently.
- **Editor scope**: keep editor-only code under `TOOLS_ENABLED` and editor init boundaries.

## GDScript conventions

- **Indentation**: tabs.
- **Naming**: `snake_case` for functions/variables, `PascalCase` for classes/nodes.
- **Coupling**: prefer signals over tight direct dependencies.

## Phoenix-specific boundaries

- Prefer edits inside `modules/ultimate_ai/`; document unavoidable non-module edits in `modules/ultimate_ai/CORE_MODIFICATIONS.md`.
- Do not make incidental source edits inside vendored submodules under `modules/ultimate_ai/external/*`.
- Keep addon bootstrap behavior deterministic (sync markers, staged addon path assumptions, plugin auto-enable flow).

## Patterns to avoid

- New global state/singletons without strong justification.
- Per-frame avoidable allocations/string churn in hot paths.
- Cross-cutting refactors unrelated to the requested change.
