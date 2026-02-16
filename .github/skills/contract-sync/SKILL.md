---
name: contract-sync
description: Synchronize API contracts and golden fixtures between Backend and Interface repos. Use when user asks to sync contracts, update fixtures, check contract compatibility, fix schema drift, or align API schemas between repos.
---

# Contract Sync — Phoenix Agentic Engine

## Context

This is the **Engine** repo (Godot fork). It consumes contracts defined by the Interface repo and fulfilled by the Backend.

The Engine does NOT own contracts — it is a consumer. The chain is:

```
Backend (owns schemas) → Interface (mirrors fixtures + typed SDK) → Engine (consumes via adapter layer)
```

## When Engine needs contract updates

| Trigger | Action |
|---------|--------|
| Backend added new API fields | Update adapter code in `modules/ultimate_ai/core/` to handle new fields |
| Backend changed response shape | Check Interface SDK first — it should have matching types |
| New endpoint added | Add adapter/handler in `modules/ultimate_ai/mcp/` |
| Breaking change (v2) | Update module code to support new version namespace |

## How to check for contract drift

1. Look at Interface repo `contracts/v1/*.json` for current fixture shapes
2. Look at Backend repo `api/schemas/` for canonical Pydantic models
3. Compare with Engine module's data handling code

## Key Engine files that depend on contracts

- `modules/ultimate_ai/core/` — orchestration hooks that send/receive backend commands
- `modules/ultimate_ai/mcp/` — MCP client that transports payloads
- `modules/ultimate_ai/shadow_tree/` — Scene Tree serialization matching Shadow Tree contract
