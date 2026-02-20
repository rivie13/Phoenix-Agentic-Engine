# PRIORITY — Current Task Context

## Active task pointer

**Read `.github/context/CURRENT_TASK.md` at the start of every session.** This file is the single source of truth for what is actively being worked on in this repo.

- The **stub template** is committed at `.github/context/CURRENT_TASK.md.stub`. This is the canonical, immutable template. Do NOT modify the stub.
- The **working copy** lives at `.github/context/CURRENT_TASK.md` and is gitignored. Each developer/agent maintains their own local copy.
- If `CURRENT_TASK.md` does not exist locally, copy the stub to create it: `cp .github/context/CURRENT_TASK.md.stub .github/context/CURRENT_TASK.md`
- If the file contains an active task: that is your primary context. All work should relate to or acknowledge this task.
- If the file says "No active task": use the `focus` skill to pick the next task from the roadmap.
- If you are spawned for a side task (bug fix, tangent): still read CURRENT_TASK.md to understand the main thread. Note in your response that the main task is X and this is a tangent.

## Project board

All tasks are tracked on the **Phoenix Project Board**: https://github.com/users/rivie13/projects/3

Board columns: Backlog → Ready → In Progress → In Review → Done

## Task lifecycle (Ralph Loop)

The task lifecycle follows the **Ralph Loop** — a conditional iteration loop. See `.github/docs/PROJECT_WORKFLOW.md` for the full diagram and documentation.

1. **Pick**: Read roadmap → select next task → create/assign issue → fill CURRENT_TASK.md → move to "In Progress"
2. **Decide**: Local IDE or Cloud Agent? If cloud, add `cloud-agent` label to auto-assign Copilot
3. **Work**: Implement the task, updating checkpoint in CURRENT_TASK.md periodically
4. **Complete**: Verify acceptance criteria → move to "In Review"/"Done" → reset CURRENT_TASK.md
5. **Next**: Repeat from step 1

## Issue hierarchy

Use GitHub sub-issues for structured work: **Epic** → **Feature** → **Task**. Sub-issues can cross repos.

## Cloud agent assignment

When an issue is labeled `cloud-agent`, the `cloud-agent-assign.yml` workflow checks that the issue is in **Ready** status on the project board. If not Ready (e.g. still in Backlog), the label is removed and the assignment is rejected. On success, the workflow assigns Copilot and automatically updates the board: Status → In Progress, Work mode → Cloud Agent. Use this for well-scoped tasks with clear acceptance criteria.

## Cross-repo coordination

When a task in this repo depends on or affects another repo:
- Note the dependency in CURRENT_TASK.md "Depends on" field
- Check the other repo's CURRENT_TASK.md to avoid conflicts
- Related repos: Backend (`rivie13/Phoenix-Agentic-Engine-Backend`), Interface (`rivie13/Phoenix-Agentic-Engine-Interface`)

## Privacy

This repo is **public**. CURRENT_TASK.md must not contain secrets, private strategy, or internal business details.
