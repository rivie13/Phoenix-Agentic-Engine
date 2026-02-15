
# Project structure — Phoenix Agentic Engine (Godot fork)

This repository is a fork of the Godot Engine with an AI module added in `modules/ultimate_ai/`.

## Repository layout

| Path | Purpose | Edit policy |
|------|---------|-------------|
| `modules/ultimate_ai/` | All Phoenix AI features | **Primary work area** |
| `modules/ultimate_ai/core/` | Client-side orchestration hooks | Active development |
| `modules/ultimate_ai/mcp/` | MCP client, server manager, tool registry | Active development |
| `modules/ultimate_ai/mcp/adapters/` | Tool adapters (pixelpen, bfxr, vcs, docs, etc.) | Active development |
| `modules/ultimate_ai/ui/` | Editor panels and controls | Active development |
| `modules/ultimate_ai/agents/` | Agent UI representations | Active development |
| `modules/ultimate_ai/external/` | Git submodules for MCP servers | Submodule references only |
| `editor/` and `editor/plugins/` | Godot editor features | Minimal hooks only |
| `core/`, `scene/`, `servers/` | Godot engine core | **Avoid editing** |
| `platform/`, `drivers/` | Platform backends | **Avoid editing** |
| `branding/` | Phoenix logos and splash art | Assets only |

## Module structure

```
modules/ultimate_ai/
├── config.py                 # Module build configuration
├── register_types.cpp/.h     # Type registration
├── SCsub                     # Build script
├── CORE_MODIFICATIONS.md     # Tracks all edits outside the module
├── core/                     # Orchestration hooks, context manager, action router
├── mcp/                      # MCP client, server manager, tool registry
│   └── adapters/             # pixelpen, bfxr, vcs, docs, github, trello, toolhive
├── ui/                       # Assistant panel, mode selector, context composer
│   ├── assistant_panel.cpp/.h
│   └── ultimate_ai_editor_plugin.cpp/.h
├── agents/                   # Agent UI representations
├── shadow_tree/              # Scene serializer, delta sync
├── worktree/                 # Git Worktree manager
├── llm/                      # LLM provider integrations
├── security/                 # Input validation, key storage
└── external/                 # Git submodules for MCP servers
```

## Fork discipline — the #1 rule

**Touch Godot core files as little as possible.** Every core edit is a future merge conflict.

| Approach | When to use | Conflict risk |
|----------|-------------|---------------|
| Module (`modules/ultimate_ai/`) | All major features, nodes, APIs | **LOW** — isolated directory |
| Editor Plugin (`editor/plugins/`) | Editor-only UI, panels | **LOW** — isolated directory |
| Core edit (`core/`, `scene/`, `editor/editor_node.cpp`) | Only if absolutely necessary for hooks | **HIGH** — document in `CORE_MODIFICATIONS.md` |

**Every edit outside `modules/ultimate_ai/` MUST be documented in `modules/ultimate_ai/CORE_MODIFICATIONS.md`** with the file path, what was changed, and why it couldn't be done in the module.

## Multi-contributor awareness

- Keep changes scoped to well-defined files and directories.
- Avoid touching shared files (`project.godot`, root `SCsub`) without coordination.
- Each feature branch should map to a specific subsystem.
- Document any cross-cutting changes clearly.
