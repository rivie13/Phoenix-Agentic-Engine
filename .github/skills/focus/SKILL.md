---
name: focus
description: Resume current work, check active task status, start a new task, or mark a task complete. Use when user says resume, what am I working on, task done, pick next task, update checkpoint, what's my focus, or starts a new session.
---

# Focus — Phoenix Agentic Engine

## Mandatory first step

Always read the current task file before doing anything else:

```
read_file(".github/context/CURRENT_TASK.md")
```

## Workflows

### Resume (user says "resume", "what am I working on?", or starts a new session)

1. Read `.github/context/CURRENT_TASK.md`
2. If a task is active: summarize the current task in 3 lines — what, last checkpoint, next step
3. If no active task: say "No active task" and offer to pick the next one

### Pick next task (user says "pick next task", "what should I work on?")

1. Read `.github/context/CURRENT_TASK.md` — confirm no active task (or ask to close current one first)
2. Read the roadmap docs:
   - `phoenix_docs_private/ROADMAP.md`
   - `.github/instructions/engine-private-roadmap.instructions.md`
3. Check the project board for items in "Ready" or "Backlog" for this repo using GitHub MCP tools:
   - `mcp_github_github_list_issues` for `rivie13/Phoenix-Agentic-Engine`
4. Recommend the highest-priority unblocked task based on: phase order, dependencies resolved, roadmap sequence
5. Ask user to confirm
6. Create a GitHub issue if one doesn't exist
7. Fill in `.github/context/CURRENT_TASK.md` with the task details
8. Move the issue to "In Progress" on the project board

### Update checkpoint (user says "save progress", "checkpoint", "update task")

1. Read `.github/context/CURRENT_TASK.md`
2. Ask what was accomplished and what's next
3. Update the "Last checkpoint" and "Next step" fields
4. Commit the updated CURRENT_TASK.md

### Complete task (user says "task done", "finished", "close task")

1. Read `.github/context/CURRENT_TASK.md`
2. Verify acceptance criteria are met
3. **Close the GitHub issue** — do NOT rely solely on `Closes #N` in the PR body. Explicitly close it:
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<N>, state="closed", stateReason="completed")
   ```
4. **Close related sub-issues** — list sub-issues of the current issue and verify each completed one is closed:
   ```
   mcp_github_github_issue_read(owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<PARENT_N>)
   ```
   For each sub-issue whose work is merged/done, close it explicitly if still open:
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="<SUB_ISSUE_REPO>", issueNumber=<SUB_N>, state="closed", stateReason="completed")
   ```
5. **Check parent epic** — if this issue was a sub-issue of an epic, read the parent epic and check whether all sibling sub-issues are now closed. If all are done, close the parent epic too.
6. Move the issue/epic to "Done" on the project board
7. Reset `.github/context/CURRENT_TASK.md` to the "no active task" state
8. Commit the reset
9. Offer to pick the next task

> **Why explicit closing?** GitHub's `Closes #N` auto-close only works when the PR merges into the repo's *default branch*. Subfeature PRs that merge into a `feature/*` branch will NOT auto-close their linked issues. Always close issues explicitly via MCP tools.

### Assign to Copilot cloud agent (user says "assign to copilot", "cloud agent this")

1. Confirm the issue is well-scoped with clear acceptance criteria
2. **Move the issue to "Ready" status on the project board** (required — the workflow rejects non-Ready issues)
3. Add the `cloud-agent` label to the issue
4. The `cloud-agent-assign.yml` workflow will:
   - Verify the issue is in Ready status (rejects Backlog / No Status / other)
   - Assign @copilot to the issue
   - Update board Status → **In Progress** automatically
   - Update board Work mode → **Cloud Agent** automatically
5. Note in CURRENT_TASK.md that this task is delegated to cloud agent

> **Do NOT** add the `cloud-agent` label to issues in Backlog — the workflow will remove the label and post a rejection comment.

## Issue hierarchy

Use sub-issues for structured work. See `.github/docs/PROJECT_WORKFLOW.md` for full details.

- **Epic** (label: `epic`) — Multi-PR milestone, maps to a `feature/*` branch, spans weeks–months
- **Feature** (label: `feature`) — Single-repo deliverable, days–weeks (may also map to a `feature/*` branch)
- **Task** (label: `task`) — Single PR unit of work, maps to a `subfeature/task/<desc>` branch, hours–days
- **Bug** (label: `bug`) — Maps to a `subfeature/bugfix/<desc>` branch
- **Refactor/Test/Docs/Chore** — Maps to `subfeature/<type>/<desc>` branches

Sub-issues can cross repos. An Engine Epic can have Backend or Interface sub-issues.

### Issue–branch mapping

| Issue type | Branch pattern | PR target |
|------------|---------------|-----------|
| Epic | `feature/<topic>` | `main` |
| Task | `subfeature/task/<desc>` | parent `feature/*` |
| Bug | `subfeature/bugfix/<desc>` | parent `feature/*` |
| Refactor | `subfeature/refactor/<desc>` | parent `feature/*` |
| Test | `subfeature/test/<desc>` | parent `feature/*` |
| Docs | `subfeature/docs/<desc>` | parent `feature/*` |
| Chore | `subfeature/chore/<desc>` | parent `feature/*` |

When picking a task, identify which epic/feature branch it belongs to and branch from that feature branch.

## Cross-repo awareness

This is the **Engine** repo (Godot fork). Related repos:
- Backend: `rivie13/Phoenix-Agentic-Engine-Backend`
- Interface: `rivie13/Phoenix-Agentic-Engine-Interface`

If the current task has cross-repo dependencies, note them in the "Depends on" field.

## Project board

- **Board URL:** https://github.com/users/rivie13/projects/3
- **Columns:** Backlog → Ready → In Progress → In Review → Done

## Reference docs

- **Full workflow:** `.github/docs/PROJECT_WORKFLOW.md` — Ralph Loop, issue hierarchy, cloud agent flow
- **Roadmap:** `phoenix_docs_private/ROADMAP.md`

## Privacy rules

This repo is **public**. When writing to CURRENT_TASK.md:
- Do NOT include API keys, secrets, or credentials
- Do NOT include private strategy details
- Keep descriptions technical and public-safe
