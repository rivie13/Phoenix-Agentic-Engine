---
name: upstream-sync
description: Sync the Godot Engine fork with the upstream godotengine/godot repository. Use when user asks to sync upstream, merge upstream changes, update the fork, resolve upstream conflicts, or check for upstream updates.
---

# Upstream Sync — Phoenix Agentic Engine (Godot Fork)

## Repo Context

- **Owner**: `rivie13`
- **Repo**: `Phoenix-Agentic-Engine`
- **Upstream**: `godotengine/godot` (branch: `master`)

## Sync Workflow

### Step 1: Fetch upstream

```bash
git fetch upstream
```

If `upstream` remote isn't configured:

```bash
git remote add upstream https://github.com/godotengine/godot.git
git fetch upstream
```

### Step 2: Reset the sync branch

```bash
git checkout upstream-sync
git reset --hard upstream/master
git push origin upstream-sync --force
```

### Step 3: Merge into main

```bash
git checkout main
git merge upstream-sync
```

### Step 4: Resolve conflicts

Conflicts are expected in areas where Phoenix touches Godot core. Priority conflict-resolution rules:

| File area | Resolution strategy |
|-----------|-------------------|
| `modules/ultimate_ai/` | **Keep ours** — this is Phoenix-only code |
| `editor/editor_node.cpp` | Carefully merge — check `CORE_MODIFICATIONS.md` for our hooks |
| `core/`, `scene/`, `servers/` | **Take upstream** unless our hook is documented in `CORE_MODIFICATIONS.md` |
| `SConstruct`, root `SCsub` | Review carefully — upstream build changes may conflict with module registration |
| `.github/workflows/` | **Keep ours** — our CI is customized |
| `branding/` | **Keep ours** — Phoenix-specific assets |

### Step 5: Verify after merge

1. Build: `C:\Python313\python.exe -m SCons platform=windows target=editor d3d12=no`
2. Run pre-commit: `C:\Python313\python.exe -m pre_commit run --all-files`
3. Launch editor binary to smoke-test

### Step 6: Push

```bash
git push origin main
```

## Automated sync workflow

There is also a GitHub Actions workflow (`upstream-sync.yml`) that can handle this automatically. Check its status:

```
mcp_github_github_actions_list(owner="rivie13", repo="Phoenix-Agentic-Engine")
```

## Key files to watch during sync

- `modules/ultimate_ai/CORE_MODIFICATIONS.md` — lists all our edits outside the module
- `editor/editor_node.cpp` — if we have hooks registered here
- `SConstruct` — module discovery
- `.gitmodules` — submodule references in `modules/ultimate_ai/external/`

## When NOT to sync

- In the middle of active feature work — finish and merge your branch first
- If upstream has a known breaking change — wait for it to stabilize
- If you have uncommitted changes — stash or commit first
