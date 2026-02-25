---
name: focus
description: Resume assigned work, confirm active task context, update checkpoints, or complete assigned tasks. Use when user says resume, what am I working on, task done, update checkpoint, what's my focus, or starts a new session.
---

# Focus — Phoenix Agentic Engine

## Mandatory first step

Always read board state first (single source of truth):

- Project board: https://github.com/users/rivie13/projects/3
- Repository: `rivie13/Phoenix-Agentic-Engine`

## Workflows

### Resume (user says "resume", "what am I working on?", or starts a new session)

1. Read board items for this repo in `In Progress` (and, if needed, `Ready`)
2. If an in-progress item exists: summarize in 3 lines — what, current state, next step
3. If no active item: say "No active assigned board task" and ask for assignment source/intent

### Confirm assigned task (dispatcher/local/cloud assignment)

1. Read assignment input from prompt/context (dispatcher payload, direct user-assigned issue, or cloud assignment)
2. Validate assignment against board state and verify lock/dependency fields (`Area`, `Depends On`, `Lock Key`, `Needed Files`)
3. Study assignment context (issue body, acceptance criteria, linked comments/PRs) before implementation
4. Confirm branch target and execution mode (Local IDE / CLI / Cloud)
5. If assignment is valid, proceed and post checkpoints during execution

### Pick next task (fallback only; user explicitly asks)

1. Confirm there is no conflicting in-progress work for this repo on the board
2. Read the roadmap docs:
   - `phoenix_docs_private/ROADMAP.md`
   - `.github/instructions/engine-private-roadmap.instructions.md`
3. Check the project board for items in "Ready" (and only use Backlog if explicitly requested)
4. Recommend the highest-priority unblocked task based on phase order and dependencies
5. Ask user to confirm assignment
6. Create a GitHub issue if one doesn't exist
7. Move the issue to **In Progress** and set its Area on the project board using signal labels:
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<N>, labels=["task", "set:status:in-progress", "set:area:<area>"])
   ```
   Valid Engine areas: `module-assistant-ui`, `module-mcp`, `module-agent`, `module-addons`, `core`, `contracts`, `infra`, `docs`, `ci`

### Update checkpoint (user says "save progress", "checkpoint", "update task")

1. Read the active board issue and latest PR/comment context
2. Ask what was accomplished and what's next
3. Post/update concise progress in issue/PR comments and board status fields

### Complete task (user says "task done", "finished", "close task")

1. Read active board issue and verify acceptance criteria are met
2. **Close the GitHub issue** — do NOT rely solely on `Closes #N` in the PR body. Explicitly close it:
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<N>, state="closed", stateReason="completed")
   ```
3. **Close related sub-issues** — list sub-issues of the current issue and verify each completed one is closed:
   ```
   mcp_github_github_issue_read(owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<PARENT_N>)
   ```
   For each sub-issue whose work is merged/done, close it explicitly if still open:
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="<SUB_ISSUE_REPO>", issueNumber=<SUB_N>, state="closed", stateReason="completed")
   ```
4. **Check parent epic** — if this issue was a sub-issue of an epic, read the parent epic and check whether all sibling sub-issues are now closed. If all are done, close the parent epic too.
5. Move the issue/epic to "Done" on the project board
6. Report completion and await/confirm next assignment

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
5. Ensure board status/work mode reflect cloud delegation and link the issue/PR context

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

When executing an assigned task, identify which epic/feature branch it belongs to and branch from that feature branch.

## Cross-repo awareness

This is the **Engine** repo (Godot fork). Related repos:
- Backend: `rivie13/Phoenix-Agentic-Engine-Backend`
- Interface: `rivie13/Phoenix-Agentic-Engine-Interface`

If the current task has cross-repo dependencies, note them in the board/issue `Depends On` field.

## Project board

- **Board URL:** https://github.com/users/rivie13/projects/3
- **Columns:** Backlog → Ready → In Progress → In Review → Done

## Reference docs

- **Full workflow:** `.github/docs/PROJECT_WORKFLOW.md` — Ralph Loop, issue hierarchy, cloud agent flow
- **Concurrency model:** `.github/docs/WORKER_FACTORY.md` — multi-agent concurrency, area definitions, conflict rules
- **Roadmap:** `phoenix_docs_private/ROADMAP.md`

## Privacy rules

This repo is **public**. In board/issue/PR updates:
- Do NOT include API keys, secrets, or credentials
- Do NOT include private strategy details
- Keep descriptions technical and public-safe
