# Integrations (Public)

Phoenix is designed to integrate both with **editor-native tools** and with **external tool servers** (for example, via Model Context Protocol-style tool interfaces).

This document is an inventory-level overview. Specific integrations may be planned or in-progress.

## In-editor integrations

These run inside (or alongside) the Godot editor experience.

- **Assistant workspace UI**: chat panel, mode selector (Ask/Plan/Agent), context composer, approval controls.
- **Pixel art tooling**: pixel-art workflows integrated into the editor (via plugin/addon integrations).
- **Diff / review UI**: surfaces proposed code changes as diffs for review.
- **Version control**: Git operations integrated into the editor (status/stage/commit workflows).
- **Terminal (optional)**: in-editor terminal surface for developer workflows.
- **SFX tooling (optional)**: simple sound-effect workflows (generate/preview/import).
- **Unit testing (GUT)**: in-editor unit test panel/workflows via bundled GUT addon sync and auto-enable bootstrap.

## External tool servers (optional)

Phoenix can connect to external tools for things like:

- **Documentation lookup** (search and display relevant docs)
- **Project management** (tasks/boards)
- **Repository hosting workflows** (issues/PRs/CI)
- **Audio or asset tooling**

The key product goal is that external tools feel like “extensions” of the editor: discoverable, permissioned, and previewable.

## Safety expectations

- Tools should be toggleable and permissioned.
- Changes should be previewed before application where feasible.
- Actions should be auditable (what ran, what changed, when).
