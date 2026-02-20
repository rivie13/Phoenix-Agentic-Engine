# Phoenix Project Workflow

This document describes the task management system used across all Phoenix repos.

## Overview

All Phoenix repos share a unified project board and task lifecycle called the **Ralph Loop** — a conditional iteration loop where each cycle picks a task, works it, completes it, and selects the next one. The loop includes a branch point for **local (IDE) vs. cloud agent** execution.



**Repos in this system:**
| Repo | Visibility | Description |
|------|-----------|-------------|
| `Phoenix-Agentic-Engine` | Public | Godot fork — the game engine |
| `Phoenix-Agentic-Engine-Backend` | Private | .NET Gateway + Python worker runtime |
| `Phoenix-Agentic-Engine-Interface` | Public | TypeScript SDK — contracts between Engine and Backend |
| `Phoenix-Agentic-Website-Frontend` | Public | Next.js public website |
| `Phoenix-Agentic-Website-Backend` | Private | .NET API for website services |

---

## The Ralph Loop

The Ralph Loop is the core task lifecycle. It is a **conditional iteration loop** — not a simple cycle, but a loop with a decision point that determines *how* each task is executed.

```
┌──────────────────────────────────────────────────────┐
│                   RALPH LOOP                         │
│                                                      │
│  ┌─────────────┐                                     │
│  │  PICK TASK   │ ◄──────────────────────────────┐   │
│  │  from board  │                                │   │
│  └──────┬───────┘                                │   │
│         │                                        │   │
│         ▼                                        │   │
│  ┌─────────────────┐                             │   │
│  │  DECISION POINT  │                            │   │
│  │  Local or Cloud? │                            │   │
│  └───┬─────────┬────┘                            │   │
│      │         │                                 │   │
│  Local IDE  Cloud Agent                          │   │
│      │         │                                 │   │
│      ▼         ▼                                 │   │
│  ┌────────┐ ┌──────────────┐                     │   │
│  │  WORK  │ │ Copilot CCA  │                     │   │
│  │  in VS │ │ works async  │                     │   │
│  │  Code  │ │ opens PR     │                     │   │
│  └───┬────┘ └──────┬───────┘                     │   │
│      │             │                             │   │
│      ▼             ▼                             │   │
│  ┌─────────────────────┐                         │   │
│  │   REVIEW + MERGE    │                         │   │
│  │   (In Review → Done)│                         │   │
│  └──────────┬──────────┘                         │   │
│             │                                    │   │
│             ▼                                    │   │
│  ┌─────────────────────┐                         │   │
│  │  RESET CURRENT_TASK │                         │   │
│  │  pick next task ────┼─────────────────────────┘   │
│  └─────────────────────┘                             │
│                                                      │
└──────────────────────────────────────────────────────┘
```

### Loop Phases

#### 1. PICK — Select the next task
- Read `.github/context/CURRENT_TASK.md` to confirm no active task
- Check the project board for items in **Ready** column
- If nothing in Ready, check **Backlog** and promote the highest-priority item
- Use the `focus` skill: say "pick next task" or "what should I work on?"
- The agent reads the roadmap, checks the board, and recommends the next task
- Create/assign a GitHub issue if one doesn't exist

#### 2. DECIDE — Local IDE or Cloud Agent?
- **Local IDE**: Complex work requiring human judgment, multi-file refactors, architecture decisions, debugging
- **Cloud Agent**: Well-scoped, self-contained tasks with clear acceptance criteria — bug fixes, test additions, doc updates, simple features
- On the project board, set the **Work mode** field to "Local (IDE)" or "Cloud Agent"
- If Cloud Agent: add the `cloud-agent` label to the issue → automation assigns Copilot

#### 3. WORK — Execute the task
- **Local path**: Agent fills in `CURRENT_TASK.md`, you work in VS Code, agent helps via chat, checkpoints are saved periodically
- **Cloud path**: Copilot coding agent works autonomously, opens a PR, you review when done
- **Tangent handling**: If a bug or side task comes up during Local work, the tangent chat reads `CURRENT_TASK.md` and knows it's a tangent — the main task context is preserved

#### 4. REVIEW — Validate and merge
- PR is opened → board item moves to **In Review**
- Code review (human or Copilot review)
- Merge when approved

#### 5. RESET — Complete and loop
- Board item moves to **Done**
- `CURRENT_TASK.md` is reset to "no active task"
- The `focus` skill offers to pick the next task
- Loop restarts

---

## Issue Hierarchy — Epics, Features, Tasks

Use GitHub's **sub-issues** feature to structure work in a hierarchy. This creates traceability from high-level goals down to individual PRs.

### Three-Level Hierarchy with Branch Mapping

```
Epic (high-level goal, spans multiple repos)
│   Branch: feature/<topic>  →  PR targets main
├── Feature (deliverable, usually one repo)
│   ├── Task  →  subfeature/task/<desc>      →  PR targets feature/<topic>
│   ├── Bug   →  subfeature/bugfix/<desc>     →  PR targets feature/<topic>
│   └── Chore →  subfeature/chore/<desc>      →  PR targets feature/<topic>
├── Feature (another repo)
│   ├── Task  →  subfeature/task/<desc>       →  PR targets feature/<topic>
│   └── Test  →  subfeature/test/<desc>       →  PR targets feature/<topic>
└── Feature
    └── Docs  →  subfeature/docs/<desc>       →  PR targets feature/<topic>
```

### Level Definitions

| Level | Scope | Lifetime | Label | Branch pattern | Example |
|-------|-------|----------|-------|----------------|---------|
| **Epic** | Multi-repo milestone | Weeks–months | `epic` | `feature/<topic>` → PR to `main` | "Phase 3: .NET Gateway + Service Bus wiring" |
| **Feature** | Single repo deliverable | Days–weeks | `feature` | (same as parent epic branch) | "Implement JWT auth middleware in Gateway" |
| **Task** | Single PR unit of work | Hours–days | `task` | `subfeature/task/<desc>` → PR to `feature/*` | "Add token validation to auth pipeline" |
| **Bug** | Bug fix within a feature | Hours–days | `bug` | `subfeature/bugfix/<desc>` → PR to `feature/*` | "Fix token expiry edge case" |
| **Refactor** | Code restructuring | Hours–days | `refactor` | `subfeature/refactor/<desc>` → PR to `feature/*` | "Extract validation into middleware" |
| **Test** | Test additions/fixes | Hours–days | `test` | `subfeature/test/<desc>` → PR to `feature/*` | "Add auth integration tests" |
| **Docs** | Documentation | Hours–days | `docs` | `subfeature/docs/<desc>` → PR to `feature/*` | "Document auth flow" |
| **Chore** | Maintenance/tooling | Hours–days | `chore` | `subfeature/chore/<desc>` → PR to `feature/*` | "Update CI pipeline" |

### How to Create the Hierarchy

1. **Create the Epic issue** in the primary repo (usually where most work happens)
   - Title: `[Epic] Phase 3: .NET Gateway + Service Bus wiring`
   - Label: `epic`
   - Add to project board

2. **Create Feature issues** as sub-issues of the Epic
   - In the Epic issue, click **"Add sub-issue"**
   - Sub-issues can be in **different repos** — an Engine Epic can have Backend sub-issues
   - Label: `feature`

3. **Create Task issues** as sub-issues of Features
   - In the Feature issue, click **"Add sub-issue"**
   - Label: `task`
   - These map 1:1 to PRs

### Cross-Repo Sub-Issues

Sub-issues work across repos. Example structure:

```
Epic #10 in Phoenix-Agentic-Engine-Backend:
  "[Epic] Phase 3: Gateway + Service Bus"
  ├── Feature #11 (Backend): "JWT auth middleware"
  │   ├── Task #12 (Backend): "Add token validation"
  │   └── Task #13 (Backend): "Add role-based access"
  ├── Feature #5 (Interface): "Auth contract types"
  │   └── Task #6 (Interface): "Add AuthToken schema"
  └── Feature #20 (Engine): "Update adapter for auth"
      └── Task #21 (Engine): "Wire new auth headers"
```

The project board shows all of these regardless of which repo they live in.

### Labels to Use

Create these labels in each repo:
- `epic` — Multi-repo milestone → maps to `feature/<topic>` branch
- `feature` — Repo-level deliverable (sub-issue of epic)
- `task` — PR-level unit of work → maps to `subfeature/task/<desc>` branch
- `bug` — Bug fix → maps to `subfeature/bugfix/<desc>` branch
- `refactor` — Code restructuring → maps to `subfeature/refactor/<desc>` branch
- `test` — Test additions/fixes → maps to `subfeature/test/<desc>` branch
- `docs` — Documentation → maps to `subfeature/docs/<desc>` branch
- `chore` — Maintenance/tooling → maps to `subfeature/chore/<desc>` branch
- `cloud-agent` — Assign to Copilot coding agent
- `blocked` — Cannot proceed (note reason in issue)

---

## Cloud Agent Auto-Assignment

When you decide a task should be handled by Copilot coding agent instead of locally in your IDE:

### How It Works

1. Create the issue in the appropriate repo
2. Add the `cloud-agent` label to the issue
3. The `cloud-agent-assign.yml` workflow fires automatically
4. The workflow assigns Copilot to the issue
5. Copilot coding agent picks up the issue, works on it, and opens a PR
6. The `project-board-sync.yml` workflow auto-adds the PR to the project board
7. You review the PR in the "In Review" column

### When to Use Cloud Agent

**Good candidates:**
- Bug fixes with clear reproduction steps
- Adding or updating tests
- Documentation updates
- Simple, well-scoped features with clear acceptance criteria
- Code formatting, linting, or hygiene tasks

**Keep local:**
- Multi-repo coordinated changes
- Architecture decisions
- Debugging complex issues requiring IDE context
- Performance optimization requiring profiling
- Anything requiring access to running services or local state

### From the Project Board

1. Move the issue to **Ready**
2. Set the **Work mode** field to "Cloud Agent"
3. Go to the issue and add the `cloud-agent` label
4. Copilot picks it up automatically

---

## Automations in This Repo

| Workflow | Trigger | Action |
|----------|---------|--------|
| `project-board-sync.yml` | Issue opened/reopened, PR opened/reopened | Adds item to Phoenix Project Board |
| `cloud-agent-assign.yml` | Issue labeled `cloud-agent` | Assigns Copilot coding agent to the issue |

---

## Files in This System

| File | Purpose |
|------|---------|
| `.github/context/CURRENT_TASK.md` | Active task pointer — read by agents at session start |
| `.github/instructions/*-current-task.instructions.md` | Instruction telling agents to read CURRENT_TASK.md |
| `.github/skills/focus/SKILL.md` | Skill for resume/pick/checkpoint/complete workflows |
| `.github/workflows/project-board-sync.yml` | Auto-add issues/PRs to project board |
| `.github/workflows/cloud-agent-assign.yml` | Auto-assign Copilot when `cloud-agent` label is added |
| `.github/docs/PROJECT_WORKFLOW.md` | This document |
