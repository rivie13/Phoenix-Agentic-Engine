---
name: phase-planning
description: Navigate the Phoenix project roadmap, understand current phase status, plan next tasks, and track what needs to be done. Use when user asks what to work on next, current project status, phase progress, roadmap, task planning, or what's left to do.
---

# Phase Planning — Phoenix Agentic Engine

## Repo Context

This is the **Engine** (Godot fork). See `phoenix_docs_private/ROADMAP.md` for the full 15-phase plan.

## Current Status

- **Phase 0**: Complete — fork, module skeleton, CI/CD, planning docs
- **Phase 1**: Current target — Minimal Assistant UI

## How to check roadmap

Read the roadmap file for full details:

```
read_file("phoenix_docs_private/ROADMAP.md")
```

## Phase 1 targets (Engine-specific)

- Wire up assistant panel shell (editor plugin registration)
- Integrate chat interface patterns from `godot-ai-autonomous-agent`
- Basic message display, text input, mode selector stub, model selection dropdown
- Key files:
  - `modules/ultimate_ai/ui/assistant_panel.cpp/.h`
  - `modules/ultimate_ai/ui/ultimate_ai_editor_plugin.cpp/.h`
  - `modules/ultimate_ai/register_types.cpp/.h`
  - `modules/ultimate_ai/config.py`

## Documentation to read before starting work

| Document | Path | Contents |
|----------|------|----------|
| Roadmap | `phoenix_docs_private/ROADMAP.md` | 15-phase delivery plan |
| Architecture | `phoenix_docs_private/ARCHITECTURE.md` | Brain-Body split, module structure |
| UX Spec | `phoenix_docs_private/UX_ASSISTANT.md` | Assistant panel UX specification |
| Integrations | `phoenix_docs_private/INTEGRATIONS.md` | Component inventory |

## Phase overview (Engine involvement)

| Phase | Engine work |
|-------|------------|
| Phase 1 | Assistant panel UI shell |
| Phase 2 | Shadow Tree serialization, delta sync |
| Phase 3 | MCP client, tool registry, adapters |
| Phase 4 | Multi-agent UI representations |
| Phase 5 | Context composer, mode selector |
| Phase 6 | Integration: pixelpen, git, diff, speech, copilot |
| Phase 14 | AI asset generation (TRELLIS, MIDI) |

## Working principles

1. Read the relevant docs before starting any phase
2. Build incrementally — validate each phase works
3. Keep all Phoenix code in `modules/ultimate_ai/`
4. Document any core edits in `CORE_MODIFICATIONS.md`
