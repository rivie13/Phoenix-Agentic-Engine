---
name: github-actions-debug
description: Debug failed GitHub Actions CI/CD workflow runs. Use when user asks to fix CI, debug a failed workflow, check why a build failed, investigate GitHub Actions errors, or troubleshoot pipeline failures.
---

# GitHub Actions Debug — Phoenix Agentic Engine

## Repo Context

- **Owner**: `rivie13`
- **Repo**: `Phoenix-Agentic-Engine`
- **Type**: Public (Godot Engine fork)

## Workflows in this repo

| Workflow file | Purpose |
|---------------|---------|
| `build.yml` | Build Phoenix Agentic Engine (Windows SCons, extension API dump) |
| `static_checks.yml` | Code style, formatting, pre-commit checks |
| `upstream-sync.yml` | Sync fork with upstream Godot |
| `windows_builds.yml` | Upstream Godot Windows builds |
| `linux_builds.yml` | Upstream Godot Linux builds |
| `macos_builds.yml` | Upstream Godot macOS builds |
| `web_builds.yml` | Upstream Godot Web builds |
| `ios_builds.yml` | Upstream Godot iOS builds |

## PR gate usage (required)

During PR work, treat failed GitHub Actions checks as a merge blocker:

1. Inspect failed runs for the PR.
2. Triage root cause using logs.
3. Fix issues locally and re-validate (`pre_commit` + relevant tests/build).
4. Push and confirm checks recover.

## Debugging Workflow: Step-by-step

### Step 1: List recent workflow runs

Use the GitHub MCP tools to find failed runs:

```
mcp_github_github_actions_list(owner="rivie13", repo="Phoenix-Agentic-Engine")
```

### Step 2: Get details of a specific run

```
mcp_github_github_actions_get(owner="rivie13", repo="Phoenix-Agentic-Engine", run_id=<RUN_ID>)
```

### Step 3: Get job logs for the failed job

```
mcp_github_github_get_job_logs(owner="rivie13", repo="Phoenix-Agentic-Engine", job_id=<JOB_ID>)
```

### Step 4: Analyze the failure

Common failure patterns in this repo:

| Failure type | Typical cause | Fix |
|-------------|---------------|-----|
| SCons build error | C++ compile errors, missing includes, linker issues | Fix the code, check `modules/ultimate_ai/SCsub` and `config.py` |
| Pre-commit / static check | Code style violations, formatting | Run `pre-commit run --all-files` locally and fix |
| Extension API dump crash | Editor binary crashes on startup | Check for null pointer or initialization issues |
| Cache restore failure | Cache key mismatch | Usually self-resolving, can ignore |
| Submodule init failures | `.gitmodules` reference issues | Check submodule URLs in `modules/ultimate_ai/external/` |

### Step 5: Fix locally and verify

1. Read the error from the job logs
2. Reproduce locally if possible using the build commands
3. Make the fix in the codebase
4. Run pre-commit: `C:\Python313\python.exe -m pre_commit run --all-files`
5. Build: `C:\Python313\python.exe -m SCons platform=windows target=editor d3d12=no`

### Step 6: Re-trigger the workflow (optional)

```
mcp_github_github_actions_run_trigger(owner="rivie13", repo="Phoenix-Agentic-Engine", workflow_id="build.yml")
```

## Important notes

- The `build.yml` workflow uses caching (`phoenix-cache-restore` action + artifacts cache) — stale cache can cause issues
- Static checks run pre-commit on changed files only
- The extension API dump step starts the editor binary headlessly — crashes here indicate runtime issues
