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

The loop supports **three execution modes** (Local IDE, CLI Agent, Cloud Agent) that can run in parallel. See `.github/docs/WORKER_FACTORY.md` for the full concurrency model.

```
┌───────────────────────────────────────────────────────────────┐
│                        RALPH LOOP                             │
│                                                               │
│  ┌─────────────┐                                              │
│  │  PICK TASK   │ ◄───────────────────────────────────────┐   │
│  │  from board  │                                         │   │
│  └──────┬───────┘                                         │   │
│         │                                                 │   │
│         ▼                                                 │   │
│  ┌──────────────────────────┐                              │   │
│  │     DECISION POINT       │                              │   │
│  │  Local / CLI / Cloud?    │                              │   │
│  └───┬─────────┬────────┬───┘                              │   │
│      │         │        │                                  │   │
│  Local IDE  CLI Agent  Cloud Agent                         │   │
│      │         │        │                                  │   │
│      ▼         ▼        ▼                                  │   │
│  ┌────────┐ ┌────────┐ ┌──────────────┐                    │   │
│  │  WORK  │ │  WORK  │ │ Copilot CCA  │                    │   │
│  │  in VS │ │ local  │ │ works async  │                    │   │
│  │  Code  │ │ wktree │ │ opens PR     │                    │   │
│  └───┬────┘ └───┬────┘ └──────┬───────┘                    │   │
│      │          │             │                            │   │
│      ▼          ▼             ▼                            │   │
│  ┌──────────────────────────────────┐                      │   │
│  │        REVIEW + MERGE            │                      │   │
│  │        (In Review → Done)        │                      │   │
│  └──────────────┬───────────────────┘                      │   │
│                 │                                          │   │
│                 ▼                                          │   │
│  ┌──────────────────────────────────┐                      │   │
│  │  CLOSE ISSUES + pick next ────┼──────────────────────┘   │
│  └──────────────────────────────────┘                          │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

### Loop Phases

#### 1. PICK — Select the next task
- Check the project board for in-progress items in this repo to confirm no conflicting active work
- Check the project board for items in **Ready** column
- If nothing in Ready, check **Backlog** and promote the highest-priority item
- Use the `focus` skill: say "pick next task" or "what should I work on?"
- The agent reads the roadmap, checks the board, and recommends the next task
- Create/assign a GitHub issue if one doesn't exist

#### 2. DECIDE — Local IDE, CLI Agent, or Cloud Agent?
- **Local IDE**: Complex work requiring human judgment, multi-file refactors, architecture decisions, debugging
- **CLI Agent**: Well-scoped tasks that benefit from local context (file system, running services) but can run in parallel with your IDE work. Invoked via `copilot-cli`. Requires a clean (committed + pushed) base branch.
- **Cloud Agent**: Self-contained tasks with clear acceptance criteria that need no local context — bug fixes, test additions, doc updates, simple features
- On the project board, set the **Work mode** field to "Local (IDE)" or "Cloud Agent" (CLI uses Local IDE)
- If Cloud Agent: add the `cloud-agent` label to the issue → automation assigns Copilot
- **Area check**: Before assigning, verify the task's area doesn't overlap with any currently active task. Use the board's Area and Lock Key fields.

#### 3. WORK — Execute the task
- **Local path**: Agent reads board issue context, you work in VS Code, agent helps via chat, checkpoints are posted to issue/PR comments
- **CLI path**: CLI agent creates its own worktree + branch, works independently, opens a PR when done
- **Cloud path**: Copilot coding agent works autonomously, opens a PR, you review when done
- **Tangent handling**: If a bug or side task comes up during Local work, the agent checks the board and knows the main task context is preserved there
- **Concurrency**: Multiple agents can work in the same repo if they touch different areas. Board fields (Area, Lock Key, Depends On) control conflict detection.

#### 4. REVIEW — Validate and merge
- PR is opened → board item moves to **In Review**
- Code review (human or Copilot review)
- Merge when approved

#### 5. RESET — Complete, close issues, and loop
- **Close the GitHub issue explicitly** using `mcp_github_github_issue_write` (state: `closed`, stateReason: `completed`)
  - Do NOT rely solely on `Closes #N` in the PR body — GitHub only auto-closes issues when merging into the default branch. Subfeature PRs merging into `feature/*` branches will NOT auto-close linked issues.
- **Close completed sub-issues** — verify each sub-issue whose work was merged is closed. Close any that remain open.
- **Close parent epic if all children are done** — if the completed task was a sub-issue, check the parent epic. If all siblings are closed, close the epic too.
- Board item moves to **Done**
- The `focus` skill offers to pick the next task
- Loop restarts

> **CRITICAL:** Never skip issue closing. A task is not complete until its GitHub issue is verified closed.

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
| `.github/docs/WORKER_FACTORY.md` | Multi-agent concurrency model — area definitions, conflict rules, agent types |
| `.github/instructions/*-current-task.instructions.md` | Instruction telling agents to read board context |
| `.github/skills/focus/SKILL.md` | Skill for resume/pick/checkpoint/complete workflows |
| `.github/workflows/project-board-sync.yml` | Auto-add issues/PRs to project board |
| `.github/workflows/cloud-agent-assign.yml` | Auto-assign Copilot when `cloud-agent` label is added |
| `.github/docs/PROJECT_WORKFLOW.md` | This document |
