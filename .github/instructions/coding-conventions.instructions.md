---
applyTo: "**"
---

# Coding conventions — Phoenix Agentic Engine (Godot fork)

## General rule

Match the style of the file you are editing — indentation, naming, include order, comment style.

## C++ conventions (Godot style)

- **Indentation**: tabs (not spaces).
- **Naming**: `snake_case` for functions and variables, `PascalCase` for classes.
- **Types**: prefer Godot core types (`String`, `Vector`, `Array`, `Dictionary`, `Variant`) over STL equivalents.
- **Error handling**: use Godot's `Error` return codes and `ERR_FAIL_*` / `ERR_*_MSG` macros — never exceptions.
- **Includes**: follow the include order used in the surrounding file (typically: own header, core headers, scene headers, module headers, thirdparty).
- **Memory**: use `memnew` / `memdelete` (Godot allocator), not raw `new` / `delete`. Ensure `memnew`/`memdelete` are paired correctly.
- **Ownership**: use `Ref<T>` for `RefCounted`-derived objects. Avoid dangling `Object *` pointers.
- **Editor guards**: editor-only code must be guarded by `#ifdef TOOLS_ENABLED` so it doesn't leak into export templates.

## GDScript conventions

- **Indentation**: tabs.
- **Naming**: `snake_case` for functions and variables, `PascalCase` for classes and nodes.
- **Signals**: prefer signals over direct coupling between components.

## Patterns to prefer

- Godot signal/callback patterns over tight coupling.
- Small, focused changes over large refactors.
- Existing Godot APIs and utilities over reimplementing functionality.

## Patterns to avoid

- New global state or singletons (unless Godot's existing pattern requires it).
- STL containers (`std::vector`, `std::string`, etc.) in Godot-integrated code.
- Allocations or string conversions in hot paths (`_process`, `_physics_process`, per-frame loops).
- C++ exceptions anywhere in engine or module code.
