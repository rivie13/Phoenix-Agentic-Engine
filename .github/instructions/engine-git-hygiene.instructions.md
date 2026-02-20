# Git hygiene and GitHub MCP workflow — Phoenix Agentic Engine

## Scope

This file defines branch, commit, validation, and pull request hygiene for the Engine repo (Godot fork).
Use it whenever making code changes, preparing pull requests, or handling review feedback.

## CLI tool policy (mandatory)

- **NEVER use `gh` CLI** — it is not installed in this environment and must not be used.
- **Always prefer GitHub MCP tools** (`mcp_github_*`) for all GitHub operations (PRs, issues, reviews, actions, branches, searches, etc.).
- Fall back to terminal `git` commands only for local worktree operations (status, add, commit, branch, checkout, rebase, push, pull, diff, log) or when MCP tools fail/are unavailable.
- Do NOT suggest or attempt `gh pr create`, `gh issue create`, `gh run list`, or any other `gh` subcommand.

## Branch hygiene

### Hierarchy

There are **three branch tiers**:

| Tier | Pattern | Branches from | Merges into | Purpose |
|------|---------|---------------|-------------|---------|
| **main** | `main` | — | — | Stable release line |
| **feature** | `feature/<topic>` | `main` | `main` | Large, multi-PR deliverable |
| **subfeature** | `subfeature/<type>/<description>` | parent `feature/*` | parent `feature/*` | Focused piece of work within a feature |

### Feature branches (`feature/*`)

- Branch from `main` for new top-level work.
- Merged into `main` **only when the entire feature is complete and validated**.
- The final `feature → main` PR will be large — that is expected and acceptable.
- Keep one focused feature per branch.

### Subfeature branches (`subfeature/*`)

Subfeature branches are the primary unit of daily work. They branch off a parent `feature/*` branch and merge back into it via small, focused PRs.

**Naming convention** — always three segments:

```
subfeature/<type>/<short-description>
```

Where `<type>` is one of:

| Type | Use for |
|------|---------|
| `task` | Planned implementation work |
| `bugfix` | Bug discovered during feature development |
| `refactor` | Structural improvement within the feature |
| `test` | Adding/improving tests for the feature |
| `docs` | Documentation specific to the feature |
| `chore` | Non-functional maintenance (deps, config, formatting) |

**Examples:**
- `subfeature/task/wire-up-assistant-panel`
- `subfeature/bugfix/fix-panel-null-crash`
- `subfeature/refactor/extract-message-renderer`
- `subfeature/test/add-panel-unit-tests`
- `subfeature/docs/update-architecture-after-panel`

### Top-level branches (non-feature work)

For small, standalone changes that do not belong to a larger feature:
- `fix/<short-topic>` — standalone bug fixes (branches from and merges to `main`)
Examples:
- `chore/<short-topic>` — non-functional maintenance
- `mcp-docs/<short-topic>` — mcp docs server work
- `docs/<short-topic>` — documentation work

### Rules

- Avoid direct commits to `main` or shared `feature/*` branches.
- Rebase/sync regularly with the parent branch to reduce merge drift.
- **Subfeature branches MUST target their parent `feature/*` branch** — never `main` directly.
- Delete subfeature branches after merge into the feature branch.

## Commit hygiene

- Run validations before commit:
  - `C:\Python313\python.exe -m pre_commit run --all-files`
  - Build: `C:\Python313\python.exe -m SCons platform=windows target=editor d3d12=no`
- Keep commits atomic and reviewable.
- Commit message style (recommended):
  - `feat: ...`
  - `fix: ...`
  - `chore: ...`
  - `test: ...`
  - `docs: ...`
- Do not include unrelated file changes in the same commit.

## PR size discipline (mandatory)

- Keep PRs small and focused — one logical change per PR.
- **Subfeature → feature PRs** should be the normal unit of review. Each should cover one discrete piece of work.
- **Feature → main PRs** will be large (accumulating all merged subfeature work). This is expected. The feature-level PR serves as the final integration gate and should summarize all subfeature PRs that were merged.
- Break work into subfeature branches early. Do not wait until a feature branch is bloated to split.
  1. Create `subfeature/<type>/<description>` branches off the parent `feature/*` branch.
  2. Open PRs from each subfeature branch into the `feature/*` branch.
  3. Once all subfeature PRs are merged and the feature is complete, open a single PR from `feature/*` into `main`.
- Target: subfeature PRs should ideally be under ~400 lines of meaningful change (excluding generated files, lock files, submodule pointer updates).
- If a subfeature PR exceeds this, strongly consider splitting into further subfeature branches.
- Never let PRs accumulate dozens of unrelated changes — this makes review impossible and increases merge risk.

## Pull request hygiene

- **Always use GitHub MCP tools** for PR operations — never `gh` CLI.
- Before creating PR:
  - Ensure branch is up to date with base
  - Ensure pre-commit and build pass
  - Confirm scope is limited to one logical change
- After creating/updating PR:
  - Check GitHub Actions/workflow run status for the PR
  - If any workflow/job fails, fetch logs and fix the root cause
  - Re-run validations locally and re-check workflow runs
  - Do not mark PR ready while required checks are failing
- PR description MUST include (use MCP tools to set):
  - **Summary**: What changed and why
  - **Changes**: Bullet list of key changes
  - **Testing**: What was tested and how, with pass/fail results
  - **Breaking changes**: Any upstream-merge or compatibility concerns
  - **Related issues**: Link related issues with `Closes #N` or `Relates to #N`
  - If core files changed: justification and `CORE_MODIFICATIONS.md` entry
- Treat Copilot review as auto-requested by default when available.
- Only request Copilot review manually when no Copilot review exists for the latest commit set on the PR.

## Review hygiene (Copilot + humans)

- Fetch and address unresolved review comments.
- For each comment:
  1. Reproduce/understand issue
  2. Apply focused fix
  3. Re-run relevant validations
  4. Respond with what changed and why
- Re-request review when follow-up changes are done.

## GitHub MCP tool preference

Prefer MCP tools over raw terminal git/GitHub commands for:
- Creating and updating PRs
- Listing PRs/reviews/comments
- Checking whether Copilot review already exists
- Requesting Copilot review only when missing for latest commits
- Reading and responding to review feedback
- Listing workflow runs/jobs and reviewing failed logs
- Creating and managing issues
- Searching for existing issues

Terminal git is still appropriate for local worktree tasks (status, branch, add/commit, rebase, push, tests).

## Issue creation and Copilot assignment (public repo)

- Create GitHub issues for trackable work items using `mcp_github_github_issue_write`.
- Use issues to break large features into smaller, trackable units of work.
- For public-facing, non-sensitive issues: assign to Copilot (cloud agent) when appropriate using `mcp_github_github_assign_copilot_to_issue`.
- Do NOT create public issues or assign to Copilot for work involving private/sensitive matters (secrets, auth internals, proprietary logic, infrastructure details, security vulnerabilities).
- Search for existing issues before creating duplicates using `mcp_github_github_search_issues`.

### Issue–branch alignment (mandatory)

Issues must mirror the branch hierarchy so that work is traceable:

| Issue type | Label | Maps to branch | Description |
|------------|-------|----------------|-------------|
| **Epic** | `epic` | `feature/<topic>` | Multi-PR deliverable; the parent issue for all subfeature work |
| **Task** | `task` | `subfeature/task/<desc>` | Planned implementation work within the feature |
| **Bug** | `bug` | `subfeature/bugfix/<desc>` | Bug found during feature development |
| **Refactor** | `refactor` | `subfeature/refactor/<desc>` | Structural cleanup within the feature |
| **Test** | `test` | `subfeature/test/<desc>` | Test coverage for the feature |
| **Docs** | `docs` | `subfeature/docs/<desc>` | Documentation for the feature |
| **Chore** | `chore` | `subfeature/chore/<desc>` | Non-functional maintenance |

- **Epic issues** should be created for each `feature/*` branch. They act as the parent tracker.
- **Sub-issues** should be created for each `subfeature/*` branch using `mcp_github_github_sub_issue_write`. Each sub-issue links to its parent epic.
- When creating a subfeature PR, reference its sub-issue with `Closes #N`.
- When merging the final `feature → main` PR, reference the parent epic with `Closes #N`.
- This creates a clear audit trail: epic → sub-issues → subfeature PRs → feature PR → main.

## Project board field management (mandatory)

The project board (rivie13/projects/3) has **separate fields** that are NOT labels:

| Field | Type | Values |
|-------|------|--------|
| **Priority** | Single select | P0 (Critical), P1 (High), P2 (Medium), P3 (Low) |
| **Size** | Single select | XS, S, M, L |
| **Work mode** | Single select | Cloud Agent, Local IDE |
| **Status** | Single select | Backlog, Ready, In Progress, In Review, Done |
| **Labels** | GitHub labels | `task`, `epic`, `feature`, `cloud-agent`, etc. |

### Labels vs project fields — never confuse them

- **Labels** are GitHub issue metadata (e.g., `task`, `cloud-agent`). They categorize the issue type and trigger workflows.
- **Project fields** (Priority, Size, Work mode, Status) are GitHub Projects V2 properties. They live on the project board, NOT on the issue.
- **NEVER create labels named after project field values** (e.g., `p0: critical`, `size: m`, `local-ide`). This was a past mistake.

### Setting project field values via signal labels

MCP tools cannot set project board fields directly. Use **signal labels** as a bridge:

1. Add transient signal labels to the issue alongside its real labels:
   ```
   mcp_github_github_issue_write(method="update", ..., labels=["task", "set:priority:p1", "set:size:m", "set:workmode:local-ide"])
   ```
2. The `sync-project-fields.yml` workflow triggers, sets the corresponding project field via GraphQL, and removes the signal label.
3. The issue ends up with only its real labels (`task`) and the correct project field values.

**Signal label format:** `set:<field>:<value>`

| Signal label | Sets field | To value |
|---|---|---|
| `set:priority:p0` | Priority | P0 |
| `set:priority:p1` | Priority | P1 |
| `set:priority:p2` | Priority | P2 |
| `set:priority:p3` | Priority | P3 |
| `set:size:xs` | Size | XS |
| `set:size:s` | Size | S |
| `set:size:m` | Size | M |
| `set:size:l` | Size | L |
| `set:workmode:cloud-agent` | Work mode | Cloud Agent |
| `set:workmode:local-ide` | Work mode | Local IDE |
| `set:status:backlog` | Status | Backlog |
| `set:status:ready` | Status | Ready |
| `set:status:in-progress` | Status | In Progress |
| `set:status:in-review` | Status | In Review |
| `set:status:done` | Status | Done |

### When to set fields

- Set Priority and Size when creating or triaging issues.
- For issues with the `cloud-agent` label, the `cloud-agent-assign.yml` workflow automatically sets Work mode → Cloud Agent and Status → In Progress. Only add `set:priority:*` and `set:size:*` signal labels for those.
- For local-IDE issues, add all relevant signal labels (`set:priority:*`, `set:size:*`, `set:workmode:local-ide`).

## Post-merge issue completion (mandatory)

After any PR is merged, **always close linked issues explicitly using MCP tools**. Do NOT rely solely on `Closes #N` in the PR body — GitHub only auto-closes issues when a PR merges into the repo's **default branch** (`main`). Subfeature PRs that merge into `feature/*` branches will NOT auto-close linked issues.

### Required steps after every PR merge:

1. **Close the linked issue:**
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<N>, state="closed", stateReason="completed")
   ```

2. **Close completed sub-issues** — if the merged PR's issue had sub-issues, verify each one whose work is also merged is closed. Close any that remain open:
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="<SUB_ISSUE_REPO>", issueNumber=<SUB_N>, state="closed", stateReason="completed")
   ```

3. **Close parent epic if all children are done** — if the closed issue was a sub-issue of an epic, read the parent epic to check whether all sibling sub-issues are now closed. If so, close the epic:
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<EPIC_N>, state="closed", stateReason="completed")
   ```

4. **Set Status → Done** on the project board using a signal label:
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<N>, labels=["task", "set:status:done"])
   ```

> **Rule:** A PR is not "fully done" until all linked issues are verified closed. This is as important as passing CI. Never skip this step.

## Engine quality gate (required before PR readiness)

- `pre_commit run --all-files` passes
- SCons build succeeds
- PR GitHub Actions checks are green (or explicitly understood/waived)
- Changes in `modules/ultimate_ai/` are expected; changes outside require `CORE_MODIFICATIONS.md` entry
- No secrets, credentials, or prompt content committed
