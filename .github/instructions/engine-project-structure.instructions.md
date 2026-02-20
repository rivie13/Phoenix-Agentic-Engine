# Project structure — Phoenix Agentic Engine (Godot fork)

This repository is a Godot fork with Phoenix editor integrations centered in `modules/ultimate_ai/`.

## Repository layout

| Path | Purpose | Edit policy |
|------|---------|-------------|
| `modules/ultimate_ai/` | Phoenix module code and integration glue | **Primary work area** |
| `modules/ultimate_ai/core/` | Backend contract adapter, terminal bridge, BFXR runtime bridge | Implemented |
| `modules/ultimate_ai/ui/` | Assistant + addon integration editor plugins | Implemented |
| `modules/ultimate_ai/external/` | Addon source submodules (PixelPen, diff, git, gdterm, GUT, BFXR) | Submodule references only |
| `modules/ultimate_ai/tools/` | Local helper scripts (`phoenix_bridge.js`) | Implemented |
| `modules/ultimate_ai/mcp/` | MCP namespace | Scaffolded |
| `modules/ultimate_ai/mcp/adapters/` | Adapter namespace | Scaffolded |
| `modules/ultimate_ai/agents/` | Agent namespace | Scaffolded |
| `editor/`, `core/`, `scene/`, `platform/`, `drivers/` | Upstream engine/editor internals | Avoid unless required |

## Module structure snapshot

```
modules/ultimate_ai/
├── config.py
├── register_types.cpp/.h
├── SCsub
├── CORE_MODIFICATIONS.md
├── core/
├── ui/
├── external/
├── tools/
├── mcp/
│   └── adapters/
└── agents/
```

## Fork discipline

- Keep Phoenix feature work in `modules/ultimate_ai/` whenever possible.
- Treat core/platform edits as high-risk and document each one in `CORE_MODIFICATIONS.md`.
- Keep build/workflow changes scoped and tied to concrete addon/runtime packaging needs.
