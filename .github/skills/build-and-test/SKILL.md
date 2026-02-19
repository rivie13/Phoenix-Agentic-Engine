---
name: build-and-test
description: Build, test, lint, and validate the Phoenix Agentic Engine (Godot fork). Use when user asks to build, compile, test, lint, run pre-commit, fix build errors, or validate changes in the Engine repo.
---

# Build & Test — Phoenix Agentic Engine (Godot Fork)

## Mandatory first step: terminal scope check

Before build/test commands, verify terminal scope:

1. `Set-Location "C:\Users\rivie\vsCodeProjects\Phoenix-Agentic-Engine"`
2. `Get-Location`
3. `git rev-parse --show-toplevel`
4. `git branch --show-current`

If scope is wrong, open a fresh Engine-scoped terminal and retry.

## Repo Identity

This is a **Godot Engine fork** (C++/SCons). The AI module lives in `modules/ultimate_ai/`.

## Build Commands

Always invoke SCons with the full Python path to avoid PATH mismatches.

### Windows (primary dev platform)

```powershell
# Build editor
C:\Python313\python.exe -m SCons platform=windows target=editor d3d12=no

# Compiled binary
bin\godot.windows.editor.x86_64.exe
```

### Other Platforms

```bash
python -m SCons platform=<platform> target=editor
```

### Build flags of note

- `module_ultimate_ai_enabled=yes` — enabled by default; set `=no` to exclude the AI module
- `d3d12=no` — skip D3D12 renderer (faster builds, avoids SDK dependency)
- `target=editor` — build the editor (not export template)

## Pre-commit (lint + format)

```powershell
# Install (one-time)
C:\Python313\python.exe -m pip install pre-commit

# Set up hooks
C:\Python313\python.exe -m pre_commit install

# Run on all files
C:\Python313\python.exe -m pre_commit run --all-files

# Run on specific files
C:\Python313\python.exe -m pre_commit run --files path/to/file.cpp path/to/another_file.gd
```

## Commit/push quality gate (required)

Before any commit/push flow:

1. Run pre-commit (all files for broad changes, targeted files for narrow fixes).
2. Run the most relevant tests for changed areas (if tests exist).
3. Run at least one build command when C++/build logic changed.
4. If hooks modify files during `git commit`, re-stage and commit again.
5. Do not bypass hooks with `--no-verify` in normal development.

Recommended terminal sequence:

```powershell
C:\Python313\python.exe -m pre_commit run --files <changed-file-1> <changed-file-2>
# Optional broader validation when scope is wide:
C:\Python313\python.exe -m pre_commit run --all-files
```

## Validation checklist

When the user asks to validate or check their work:

1. **Pre-commit passes** — run `pre_commit run --all-files`
2. **Build succeeds** — run the SCons build command above
3. **Binary launches** — confirm `bin\godot.windows.editor.x86_64.exe` starts without crash
4. **No regressions** — if editing core files, check that existing functionality still works

## Common build errors and fixes

| Error | Fix |
|-------|-----|
| `python not found` | Use full path: `C:\Python313\python.exe` |
| `scons not found` | Install via `C:\Python313\python.exe -m pip install scons` |
| Missing includes after module edits | Check `config.py` and `SCsub` in `modules/ultimate_ai/` |
| Linker errors in module | Verify `register_types.cpp` exports match header declarations |
| D3D12 SDK missing | Add `d3d12=no` to the build command |

## Workflow

1. Make code changes
2. Run pre-commit on changed files
3. Build with SCons
4. Launch editor binary to smoke-test
5. Report: what was built, what was tested, platform used
