---
name: load-engine-instructions
description: Load Engine repo instruction files into context. Use when working on Engine code and needing coding conventions, build instructions, project structure, architecture, roadmap, or strategy context for the Godot fork.
---

# Load Engine Instructions

## When to use

When working on code in the **Phoenix-Agentic-Engine** repo (Godot fork) and you need repo-specific context. The agent should read the relevant instruction files to understand conventions, architecture, and project structure.

## Available instruction files

All files live in `Phoenix-Agentic-Engine/.github/instructions/`:

| File | Content | When to load |
|------|---------|-------------|
| `engine-coding-conventions.instructions.md` | C++ style, Godot patterns, naming | When writing or reviewing code |
| `engine-build-and-test.instructions.md` | SCons build, pre-commit, validation | When building, testing, or fixing build errors |
| `engine-project-structure.instructions.md` | Directory layout, module organization | When navigating the codebase or adding new files |
| `engine-private-architecture.instructions.md` | Brain-Body split, module internals | When making architectural decisions |
| `engine-private-roadmap.instructions.md` | Phase plan, integrations, milestones | When planning work or checking status |
| `engine-private-strategy.instructions.md` | Client vs backend boundary decisions | When deciding where code should live |

**Also available**: `engine-code-review.instructions.md` — manual-only, load when reviewing PRs. Has `excludeAgent` guard to block the autonomous coding agent.

## How to load

Read the files you need using `read_file`. Common patterns:

### Starting coding work
```
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-coding-conventions.instructions.md")
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-project-structure.instructions.md")
```

### Build/test troubleshooting
```
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-build-and-test.instructions.md")
```

### Architecture/planning
```
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-private-architecture.instructions.md")
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-private-roadmap.instructions.md")
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-private-strategy.instructions.md")
```

### Load all (when full context is needed)
```
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-coding-conventions.instructions.md")
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-build-and-test.instructions.md")
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-project-structure.instructions.md")
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-private-architecture.instructions.md")
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-private-roadmap.instructions.md")
read_file("Phoenix-Agentic-Engine/.github/instructions/engine-private-strategy.instructions.md")
```
