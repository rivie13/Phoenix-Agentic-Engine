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

VS Code task (preferred in this repo):

- `dev: precommit: check`
	- Leave prompt empty to run `--all-files` (default).
	- Provide space-separated file paths to run `--files ...`.
	- For `--all-files`, treat it as long-running and wait for task completion before moving to any next validation step.

## Git hygiene before commit/push

- Use terminal Git commands from the Engine repo (`git add`, `git commit`, `git push`) so local hooks run consistently.
- Do not bypass hooks with `--no-verify` in normal development.
- If `git commit` auto-runs pre-commit hooks and they modify files, re-stage those files and re-run `git commit`.
- Before pushing, run either:
	- full checks: `dev: precommit: check` (empty input) or `C:\Python313\python.exe -m pre_commit run --all-files`
	- targeted checks for touched files: `dev: precommit: check` (provide files) or `C:\Python313\python.exe -m pre_commit run --files <file1> <file2>`
- Treat any pre-commit failure as a block for push until fixed.

## VS Code validation tasks (separate from pre-commit)

Run these tasks as an additional gate before commit/push:

- `dev: verify: check` — runs `dev: build editor` then headless version/startup smoke checks.
- `dev: verify: check (full)` — same as above plus headless doctool check to a temp output directory.

Available focused tasks:

- `dev: precommit: check`
- `dev: verify: headless:version`
- `dev: verify: headless:startup`
- `dev: verify: headless:doctool-temp`

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
- Run `dev: verify: check` (or `dev: verify: check (full)` when touching doc-exposed/editor-surface code) before commit/push.
