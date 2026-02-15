
# Build and test — Phoenix Agentic Engine (Godot fork)

## Building (Windows)

Always invoke SCons with the full Python path to avoid PATH mismatches:

```powershell
# Build editor (from repo root)
C:\Python313\python.exe -m SCons platform=windows target=editor d3d12=no

# Compiled binary location
bin\godot.windows.editor.x86_64.exe
```

For other platforms, use the standard Godot SCons workflow:

```bash
python -m SCons platform=<platform> target=editor
```

## Pre-commit

```powershell
# Install
C:\Python313\python.exe -m pip install pre-commit

# Set up hooks
C:\Python313\python.exe -m pre_commit install

# Run manually
C:\Python313\python.exe -m pre_commit run --all-files

# Run on specific files
C:\Python313\python.exe -m pre_commit run --files path/to/file.cpp path/to/another_file.gd
```

## Upstream sync workflow

```bash
git fetch upstream
git checkout upstream-sync
git reset --hard upstream/master
git push origin upstream-sync --force
git checkout main
git merge upstream-sync
# Resolve conflicts, then push
git push origin main
```

## Validation expectations

- PRs should include what was built, what was run, and what platform was tested.
- Tests must be deterministic and must not require network access.
- Build must succeed with the standard SCons command above before merging.
