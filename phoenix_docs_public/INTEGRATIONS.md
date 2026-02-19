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

## Web MVP prototype scope (current planning target)

For the first exported web prototype, the target scope is intentionally narrow:

- **Assistant chat workflow** only (Ask/Plan/Agent interaction path).
- **Pixel art workflow** via PixelPen only.

The web MVP is expected to keep other editor integrations out of scope until parity and reliability are proven.

Access model requirement for web MVP:

- Web access to Phoenix runtime/orchestration features is **account-gated**.
- Unauthenticated users should not be able to reach protected Phoenix runtime endpoints.
