---
name: cross-repo-impact
description: Understand when a change in one Phoenix repo requires changes in the other repos. Use when user asks about cross-repo dependencies, what other repos need updating, impact analysis, or when making changes that affect the Engine-Backend-Interface boundary.
---

# Cross-Repo Impact Analysis — Phoenix Agentic Engine

## Three-Repo Model

| Repo | Role | Visibility |
|------|------|------------|
| **Engine** (this repo) | The Body — Godot fork, UI shell, local tool executors | Public |
| **Interface** | The Nervous System — versioned API contracts, typed SDK | Public |
| **Backend** | The Brain — orchestration, prompts, model routing | Private |

## Impact matrix: "If I change X, what else needs updating?"

### Changes in THIS repo (Engine)

| What changed in Engine | Interface impact | Backend impact |
|----------------------|-----------------|----------------|
| Added new UI panel in `modules/ultimate_ai/ui/` | None | None |
| Changed Shadow Tree serialization format | Update contract fixtures | Update Shadow Tree ingest/patch |
| Added new MCP tool adapter | None | Backend needs to know about new tool |
| Changed scene tree snapshot format | Update delta contract types | Update session delta handling |
| Modified command executor interface | Update SDK command types | Update command emission format |
| New Godot node type exposed to AI | None | Backend needs node in tool registry |

### Changes in Backend that affect Engine

| What changed in Backend | Engine response needed |
|------------------------|----------------------|
| New API response field | Update adapter code to handle new field |
| New command type | Add command executor handler |
| Changed approval flow | Update approval UI |
| New tool registered | May need new MCP adapter stub |
| WebSocket event format change | Update PubSub client code |

### Changes in Interface that affect Engine

| What changed in Interface | Engine response needed |
|--------------------------|----------------------|
| New contract fixture | Update adapter to match new shape |
| Type breaking change (v2) | Update SDK integration layer |
| New transport error type | Handle new error in MCP client |

## The Golden Rule

> **If it makes us money or makes us unique, it lives in the Backend.**
> **If it enables the community to contribute, it lives in the Engine.**
> **If it defines the contract between them, it lives in the Interface.**

## Checklist for cross-repo changes

1. Identify which repos are affected using the impact matrix above
2. Start with the **canonical source** (Backend for schemas, Engine for UI)
3. Update the **Interface** fixtures/types if contracts changed
4. Update the **consuming** repo's adapter/handler code
5. Run tests in ALL affected repos before merging
6. Coordinate PR timing — don't merge one side without the other

## Files to check in this repo when other repos change

- `modules/ultimate_ai/core/` — orchestration hooks, action router
- `modules/ultimate_ai/mcp/` — MCP client, tool registry
- `modules/ultimate_ai/shadow_tree/` — scene serialization
- `modules/ultimate_ai/ui/` — UI that renders backend responses
