---
name: update-copilot-instructions
description: Update Copilot instruction files to reflect current project state, architecture, phase progress, or conventions. Use when user asks to update instructions, refresh copilot context, sync instruction files with current reality, or update architecture/roadmap documentation in instruction files.
---

# Update Copilot Instructions — Phoenix Agentic Engine

## Repo Context

- **Repo**: `Phoenix-Agentic-Engine` (Godot fork — public)
- **Instruction location**: `.github/instructions/`

## Instruction files in this repo

| File | Purpose | Auto-loads? |
|------|---------|-------------|
| `engine-code-review.instructions.md` | PR review priorities | No (manual, `excludeAgent: "coding-agent"`) |
| `engine-coding-conventions.instructions.md` | C++/GDScript style guide | No (manual) |
| `engine-build-and-test.instructions.md` | SCons commands, pre-commit, validation | No (manual) |
| `engine-project-structure.instructions.md` | Repo layout, module organization | No (manual) |
| `engine-private-architecture.instructions.md` | Three-repo model, MCP hosting, data flow | No (manual) |
| `engine-private-roadmap.instructions.md` | Phase status, integration inventory | No (manual) |
| `engine-private-strategy.instructions.md` | Client-vs-backend boundary decisions | No (manual) |

## How to update

### Step 1: Identify what changed

- Phase advanced? → Update `engine-private-roadmap.instructions.md`
- Architecture changed? → Update `engine-private-architecture.instructions.md`
- Repo structure changed? → Update `engine-project-structure.instructions.md`
- Build process changed? → Update `engine-build-and-test.instructions.md`
- Conventions changed? → Update `engine-coding-conventions.instructions.md`
- Client/backend boundary shifted? → Update `engine-private-strategy.instructions.md`
- Review priorities changed? → Update `engine-code-review.instructions.md`
- Git/branch/pre-commit workflow changed? → Update `engine-build-and-test.instructions.md`, `engine-coding-conventions.instructions.md`, and relevant skills (`build-and-test`, `pr-management`, `github-code-review`)

### Step 2: Read the current file and update it

Use `read_file` to load, then edit with current state.

If workflow behavior changes, keep these three layers aligned in the same PR when possible:

1. Contributor docs (`README.md` and/or `CONTRIBUTING.md`)
2. Instruction files (`.github/instructions/*.instructions.md`)
3. Skills (`.github/skills/*/SKILL.md`)

### Step 3: Keep instructions concise

- State facts, not procedures (procedures go in skills)
- Use tables over prose
- Avoid duplicating skill content
- Target under 50 lines per file

### Step 4: Cross-repo consistency

If the change affects the three-repo model, also update corresponding files in:
- `Phoenix-Agentic-Engine-Backend/.github/instructions/`
- `Phoenix-Agentic-Engine-Interface/.github/instructions/`

### Step 5: Verify no secrets leaked (PUBLIC repo)

This is a **public** repo. Instruction files must NOT contain:
- System prompts or prompt templates
- API keys or credentials
- Internal pricing or billing details
- Orchestration implementation details

The `engine-private-*.instructions.md` files contain architecture-level details — verify they're appropriate for public visibility. Truly sensitive content belongs in `phoenix_docs_private/` (gitignored) or the Backend repo (private).

## Instruction file format

### Manual-only (most files)
```markdown
# Title

Content here. Keep it concise.
```
No frontmatter = manual-only. Agent loads these via the `load-engine-instructions` skill.

### With excludeAgent guard (code-review only)
```markdown
---
excludeAgent: "coding-agent"
---

# Title

Content here.
```
The `excludeAgent` guard prevents the autonomous coding agent from using this file even if manually attached.
