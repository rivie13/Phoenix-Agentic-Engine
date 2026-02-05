# Copilot Instructions (Phoenix Agentic Engine)

Phoenix Agentic Engine is a fork of Godot with AI-native, agentic workflows layered in. Prefer minimal, well-isolated changes and align with existing Godot patterns in the same subsystem.

## Working Principles
- Keep core engine changes small and localized; prefer editor tooling, plugins, or modules when possible.
- Match nearby code conventions and patterns in the file you are editing.
- Avoid broad refactors unless explicitly requested.

## Key Areas (Orientation)
- Editor features: `editor/` and `editor/plugins/`
- Engine core: `core/`, `scene/`, `servers/`
- Platform backends: `platform/`, `drivers/`
- Documentation: `phoenix_docs/`

## Build & Run (Windows)
- Always invoke SCons with the **full Python path** to avoid PATH mismatches.
- Current Python path on this machine: `C:\Python313\python.exe`
- Build command (from repo root):
  - `C:\Python313\python.exe -m SCons platform=windows target=editor d3d12=no`
- The compiled editor binary is:
  - `bin\godot.windows.editor.x86_64.exe`

## Pre-commit
- Use the same Python path as the build for all `pre-commit` commands.
- Install:
  - `C:\Python313\python.exe -m pip install pre-commit`
- Hook setup:
  - `C:\Python313\python.exe -m pre_commit install`
- Run manually:
  - `C:\Python313\python.exe -m pre_commit run --all-files`

## Documentation Pointers
- Build guide: [phoenix_docs/BUILDING.md](../phoenix_docs/BUILDING.md)
- Pre-commit guide: [phoenix_docs/PRECOMMIT.md](../phoenix_docs/PRECOMMIT.md)
- Repo overview: [README.md](../README.md)
