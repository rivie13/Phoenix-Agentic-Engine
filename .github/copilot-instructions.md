# Copilot Instructions (Phoenix Agentic Engine)

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

## Docs to reference
- Build guide: [phoenix_docs/BUILDING.md](../phoenix_docs/BUILDING.md)
- Pre-commit guide: [phoenix_docs/PRECOMMIT.md](../phoenix_docs/PRECOMMIT.md)
