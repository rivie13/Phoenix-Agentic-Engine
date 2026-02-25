# Worker Factory — Multi-Agent Concurrency Model

This document describes how multiple agents (Local IDE, CLI agent, Cloud agent) work concurrently across and within Phoenix repos without stepping on each other.

## Overview

The Ralph Loop (see `PROJECT_WORKFLOW.md`) is the task lifecycle. The **Worker Factory** extends it to support **parallel execution** — multiple agents working simultaneously on different tasks, even within the same repo.

**Key principles:**
1. **GitHub project board is the single source of truth** — board fields (Area, Lock Key, Depends On) control concurrency
2. **Area-level exclusion** — only one active task per area; cross-area tasks run in parallel
3. **Agents check the board on startup** — query active items in this repo and compare areas before starting
4. **Merge queue on protected branches** — GitHub enforces sequential integration as a safety net
5. **The human reviews dispatch state, not every line of code** — role shifts to traffic controller + spot auditor

---

## Agent Types

There are **three execution modes** for working on tasks. Each has different strengths and appropriate use cases.

| Mode | Where it runs | Branch behavior | Best for |
|------|--------------|-----------------|----------|
| **Local IDE** | VS Code, human + Copilot chat | Works on existing worktree checkout | Complex work, architecture, debugging, multi-file refactors |
| **CLI Agent** | `copilot-cli` invoked locally | Creates its own worktree/branch from a clean base | Well-scoped tasks needing local context (file system, services) |
| **Cloud Agent** | GitHub Copilot coding agent (remote) | Creates its own branch on GitHub | Self-contained tasks with clear acceptance criteria |

### When to use which

| Scenario | Mode | Why |
|----------|------|-----|
| Architecture decisions, multi-repo coordination | Local IDE | Needs human judgment and cross-repo visibility |
| Debugging with running services | Local IDE | Needs local state, breakpoints, logs |
| Bug fix with clear reproduction steps | Cloud Agent | Self-contained, agent can work autonomously |
| Adding tests for existing code | Cloud Agent or CLI | Clear scope, low conflict risk |
| Documentation updates | Cloud Agent | No conflict risk, clear acceptance criteria |
| Feature implementation touching few files | CLI Agent | Can run locally in parallel while you work on something else |
| Code formatting, linting, hygiene | Cloud Agent | Trivial scope, agent excels at these |
| Task in an area you're NOT actively touching | CLI or Cloud | No overlap with your local work |

### CLI Agent — clean worktree requirement

When invoking a CLI agent locally, it creates a new worktree from the specified branch. **The base branch must be clean** (all changes committed and pushed). If you have uncommitted changes on the base branch, the CLI agent worktree will be in a dirty/inconsistent state.

**Before invoking CLI agent:**
1. Commit or stash all local changes on the base branch
2. Push the base branch to origin
3. Then invoke the CLI agent, which creates its own worktree + branch

---

## Area-Level Concurrency Control

### What is an area?

An area is a coarse-grained partition of the codebase. Each repo defines its own areas based on directory structure. Only one active task may touch a given area at a time.

### Area definitions by repo

#### Phoenix-Agentic-Engine
| Area | Directories | Notes |
|------|-------------|-------|
| `module/assistant-ui` | `modules/ultimate_ai/assistant/` | Chat panel, UI components |
| `module/mcp` | `modules/ultimate_ai/mcp/` | MCP client, adapters |
| `module/agent` | `modules/ultimate_ai/agent/` | Agent orchestration |
| `module/addons` | `modules/ultimate_ai/addons/` | Plugin integrations |
| `core` | `core/`, `scene/`, `editor/`, `main/` | Engine core (high conflict risk) |
| `docs` | `doc/`, `phoenix_docs_*` | Documentation (low conflict risk) |
| `ci` | `.github/workflows/`, `.github/` | CI/CD and tooling |

#### Phoenix-Agentic-Engine-Backend
| Area | Directories | Notes |
|------|-------------|-------|
| `gateway` | `gateway/` | .NET Gateway API |
| `worker` | `worker/` | Python worker runtime |
| `orchestrator` | `orchestrator/` | Agent orchestration layer |
| `contracts` | `contracts/` | API contracts/schemas |
| `infra` | `infra/` | Infrastructure-as-code |
| `docs` | `docs/`, `backend_docs/` | Documentation |
| `ci` | `.github/workflows/`, `scripts/` | CI/CD and tooling |

#### Phoenix-Agentic-Engine-Interface
| Area | Directories | Notes |
|------|-------------|-------|
| `sdk/client` | `sdk/client/` | Client SDK modules |
| `sdk/core` | `sdk/` (root-level exports) | Core SDK types/index |
| `contracts` | `contracts/` | Contract schemas |
| `tests` | `tests/` | Test suite |
| `docs` | `docs/` | Documentation |
| `ci` | `.github/workflows/`, `scripts/` | CI/CD and tooling |

#### Phoenix-Agentic-Website-Frontend
| Area | Directories | Notes |
|------|-------------|-------|
| `app/pages` | `src/app/` | Next.js pages/routes |
| `components` | `src/components/` | React components |
| `content` | `content/` | Blog/CMS content |
| `public` | `public/` | Static assets |
| `docs` | `docs/` | Documentation |
| `ci` | `.github/workflows/` | CI/CD |

#### Phoenix-Agentic-Website-Backend
| Area | Directories | Notes |
|------|-------------|-------|
| `api` | `src/Phoenix.Agentic.Website.Backend.Api/` | API project |
| `domain` | `src/Phoenix.Agentic.Website.Backend.Domain/` | Domain models |
| `infra` | `src/Phoenix.Agentic.Website.Backend.Infrastructure/` | Data/infra layer |
| `tests` | `tests/` | Test projects |
| `docs` | `docs/` | Documentation |
| `ci` | `.github/workflows/`, `scripts/` | CI/CD |

### Exclusion rules

1. **One active task per area** — if board item #42 has `Area: gateway` and is In Progress, no other task may claim `Area: gateway` until #42 is done
2. **Cross-area parallelism is allowed** — `TASK-42.md` (gateway) and `TASK-57.md` (worker) can run concurrently
3. **`docs` and `ci` areas are low-risk** — multiple tasks can touch these if they edit different files, but agents should still check
4. **Cross-repo parallelism is unlimited** — tasks in different repos never conflict at the file level

### How agents check for conflicts

**On startup (before writing code):**

1. Query the project board for items with Status = "In Progress" or "Claimed" in this repo
2. Read each active item's **Area** and **Lock Key** fields
3. Compare your intended area against each active item's declared area
4. If overlap exists:
   - **STOP** — do not proceed
   - Report: "Area conflict: Issue #N is already active in area X. Cannot start."
   - Options: wait for that task to complete, re-scope to avoid the area, or escalate to the human
5. If no overlap: proceed — set your board item's Area field and move Status to In Progress

Also check the **Depends On** field — if your item depends on another that isn't Done yet, wait for it to complete first.

---

## The Human's Role — Traffic Controller

In the worker factory model, the human's role changes from "code reviewer for every PR" to **traffic controller and spot auditor**.

### What the human does

| Activity | Frequency | Purpose |
|----------|-----------|---------|
| Triage issues, set priority/size on project board | Daily | Keeps the queue healthy |
| Decide agent type (local/CLI/cloud) for each task | Per task | Match task complexity to agent capability |
| Review dispatch state (board active items) | A few times per day | Ensure no conflicts, no stale tasks |
| Spot-audit PRs — read diffs for high-risk changes | As needed | Trust but verify |
| Resolve area conflicts when agents report them | As needed | Break ties, re-scope work |
| Merge queue management | As needed | Ensure PRs merge in safe order |

### What the human does NOT need to do

- Read every line of every PR (Copilot review handles routine checks)
- Manually create branches (agents do this)
- Manually create PRs (agents do this via MCP tools)
- Manually close issues (agents do this via MCP tools)

### Tiered review policy

| Risk level | Review requirement | Auto-merge? |
|------------|-------------------|-------------|
| Low (docs, tests, content, chore) | Copilot review only | Yes, if CI passes |
| Medium (feature work in isolated area) | Copilot review + human spot-check | No — human clicks merge |
| High (core changes, cross-area, architecture) | Full human review | No — human reads the diff |

Risk level is determined by the area and task type. `docs` and `ci` areas are low risk. `core` area in Engine is always high risk. Everything else is medium.

---

## Branch and PR Hygiene — Agent Responsibilities

Agents MUST handle their own branch and PR lifecycle. The human should not have to manually create branches, open PRs, or close issues. This is the #1 operational hygiene rule.

### What every agent MUST do (any mode)

1. **Branch from the correct base** — subfeature branches from `feature/*`, not `main`
2. **Create the branch before starting work** — use `git checkout -b subfeature/task/desc` or MCP tools
3. **Commit frequently** — atomic commits with conventional message style (`feat:`, `fix:`, `chore:`, etc.)
4. **Push to origin** — do not leave work only on local branches
5. **Open a PR** when work is complete — use MCP tools (`mcp_github_github_create_pull_request`)
6. **Fill in PR description** — summary, changes, testing, related issues
7. **Request Copilot review** if not auto-triggered
8. **Close the issue explicitly** after PR merge — use `mcp_github_github_issue_write`
9. **Delete the branch** after merge (GitHub can auto-delete, but verify)

### What agents should NOT wait for the human to do

- Creating the branch (agent creates it)
- Opening the PR (agent opens it via MCP)
- Writing the PR description (agent writes it)
- Closing the issue (agent closes it via MCP)
- Moving the board card (agent uses signal labels)

The human's only required action is **merging** (for medium/high risk) and **resolving conflicts** if they arise.

---

## Putting It All Together — End-to-End Flow

### Scenario: Three agents working in parallel on Backend

```
Human (local IDE):     Working on #42 — gateway/auth refactor     Area: gateway
CLI agent (worktree):  Working on #57 — worker telemetry logging   Area: worker
Cloud agent (remote):  Working on #63 — add contract test fixtures Area: contracts
```

**Step 1 — Human starts #42 locally:**
- Board item #42 has Area: gateway, Status: In Progress
- Works in VS Code as usual

**Step 2 — Human dispatches #57 to CLI:**
- Commits and pushes current gateway work (clean base!)
- Invokes CLI agent for issue #57
- CLI agent checks board → #42 is active in gateway, #57 wants worker → no overlap → proceeds
- CLI agent creates worktree, branch `subfeature/task/worker-telemetry`, and starts working

**Step 3 — Human dispatches #63 to cloud:**
- Adds `cloud-agent` label to issue #63
- Cloud agent checks board → #42 (gateway) and #57 (worker) active → #63 wants contracts → no overlap → proceeds
- Cloud agent works on its own branch, opens PR when done

**Step 4 — PRs arrive:**
- #57 PR lands first → Copilot review passes → CI passes → human merges (medium risk) → issue closed
- #63 PR lands second → Copilot review passes → CI passes → human merges → issue closed
- #42 finishes last → human opens PR → review → merge → issue closed

All three tasks completed concurrently, no conflicts.

---

## FAQ

**Q: What if an agent needs files outside its declared area?**
A: Update the board item's Area field to include the expanded area. If the new area overlaps with another active item, stop and report the conflict.

**Q: What if two PRs modify the same file but in different areas?**
A: This shouldn't happen if areas are defined correctly. If it does, the merge queue catches it — the second PR must rebase and pass CI.

**Q: Can I have two local IDE tasks in the same repo at once?**
A: Not recommended. The local IDE agent typically works on one task at a time. Use CLI or cloud agents for parallel work alongside your local task.

**Q: Does the cloud agent have access to local state?**
A: No — cloud agents work on GitHub, not your local filesystem. They rely on the project board for conflict awareness and the issue body for task context.
