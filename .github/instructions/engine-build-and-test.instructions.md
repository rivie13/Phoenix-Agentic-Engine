# Build and test — Phoenix Agentic Engine (Godot fork)

## Repo-scoped terminal/tool discipline (required)

- In this multi-repo workspace, only run Engine commands from the Engine repo root:
	- `C:\Users\rivie\vsCodeProjects\Phoenix-Agentic-Engine`
- Before running build/test/tools, verify scope in the active terminal:
	- `Get-Location`
	- `git rev-parse --show-toplevel`
	- `git branch --show-current`
- If repo root or branch is wrong, open a fresh terminal for this repo and re-run the checks.
- Do not run Engine scripts from Backend or Interface terminal contexts.

## Canonical Windows build

Always invoke SCons with the pinned Python path:

```powershell
C:\Python313\python.exe -m SCons platform=windows target=editor d3d12=no
```

Primary Windows editor binaries:
- `bin\phoenix_agentic.windows.editor.x86_64.exe`
- `bin\phoenix_agentic.windows.editor.x86_64.console.exe`

## Pre-commit

```powershell
C:\Python313\python.exe -m pip install pre-commit
C:\Python313\python.exe -m pre_commit install
C:\Python313\python.exe -m pre_commit run --all-files
```

## Phoenix addon packaging expectations

Editor artifact validation should include these staged paths when relevant:
- `bin/addons/net.yarvis.pixel_pen`
- `bin/addons/diff-margin`
- `bin/addons/godot-git-plugin`
- `bin/addons/gdterm`
- `bin/addons/gut`
- `bin/addons/bfxr2-mcp-server`
- `bin/tools/node/<platform>/...` (bundled runtime for BFXR bridge)

## Validation expectations

- Include build/test commands and platform in PR notes.
- Keep tests deterministic and local-only.
- Verify changed addon integrations still stage and bootstrap in editor builds.
