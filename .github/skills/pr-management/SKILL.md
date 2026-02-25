---
name: pr-management
description: Create, update, and manage GitHub pull requests across Phoenix repos. Use when user asks to create a PR, update a PR description, push changes, list PRs, merge a PR, check PR status, or manage branches.
---

# PR Management — Phoenix Agentic Engine

## CLI tool policy (mandatory)

- **NEVER use `gh` CLI** — it is not installed and must not be used.
- **Always prefer GitHub MCP tools** (`mcp_github_*`) for all GitHub operations.
- Fall back to terminal `git` commands only for local worktree operations or when MCP tools fail.
- Do NOT suggest or attempt any `gh` subcommand.

## Repo Context

- **Owner**: `rivie13`
- **Repo**: `Phoenix-Agentic-Engine`
- **Canonical integration base**: Determined by branch tier:
  - Subfeature branches → parent `feature/*` branch
  - Feature branches → `main`
  - Standalone fix/chore branches → `main`

## Branch hygiene (required)

### Hierarchy

| Tier | Pattern | Branches from | Merges into |
|------|---------|---------------|-------------|
| **main** | `main` | — | — |
| **feature** | `feature/<topic>` | `main` | `main` |
| **subfeature** | `subfeature/<type>/<desc>` | parent `feature/*` | parent `feature/*` |

Subfeature `<type>` values: `task`, `bugfix`, `refactor`, `test`, `docs`, `chore`.

1. Start from the latest target base branch (feature branch for subfeatures, main for features).
2. Create a focused topic branch with the appropriate naming convention.
3. Keep PR scope narrow; avoid unrelated file changes.
4. Run pre-commit + relevant tests before push.
5. Never force-push shared branches unless explicitly coordinated.
6. **Subfeature PRs MUST target their parent `feature/*` branch** — never `main`.

## PR size discipline (mandatory)

- Keep PRs small and focused — one logical change per PR.
- **Subfeature → feature PRs** are the normal unit of review.
- **Feature → main PRs** will be large (accumulating all merged subfeature work). This is expected.
- Break work into subfeature branches early:
  1. Create `subfeature/<type>/<description>` branches off the parent `feature/*` branch.
  2. Open PRs from each subfeature branch into the `feature/*` branch.
  3. Once all subfeature PRs are merged, open a single PR from `feature/*` into `main`.
- Target: subfeature PRs should ideally be under ~400 lines of meaningful change (excluding generated files, lock files, submodule pointer updates).
- If a subfeature PR exceeds this, strongly consider splitting before requesting review.
- Never let PRs accumulate dozens of unrelated changes.

Example setup (subfeature workflow):

```bash
git checkout feature/assistant-panel
git pull --rebase origin feature/assistant-panel
git checkout -b subfeature/task/wire-up-panel-shell
```

Example setup (feature workflow):

```bash
git checkout main
git pull --rebase origin main
git checkout -b feature/assistant-panel
```

## Create a Pull Request

### Step 1: Check for PR template

Look for a PR template before creating:

```
file_search(".github/PULL_REQUEST_TEMPLATE*.md")
file_search(".github/PULL_REQUEST_TEMPLATE/**")
```

Also check `.github/PULL_REQUEST_TEMPLATE/` directory.

### Step 2: Get current branch changes

Use `get_changed_files` to see what's modified.

### Step 3: Push and create the PR with MCP tools

Push branch from terminal:

```bash
git push -u origin <feature-branch>
```

Create PR via MCP tool:

```
mcp_github_github_create_pull_request(
  owner="rivie13",
  repo="Phoenix-Agentic-Engine",
  title="<descriptive title>",
  body="<use template if found>",
  head="<branch-name>",
  base="<parent branch>"   # feature/* for subfeatures, main for features
)
```

### PR description should include (per repo conventions):
- **Summary**: What was built/changed and why
- **Changes**: Bullet list of key changes
- **Testing**: What was run/tested, platform, and pass/fail results
- **Breaking changes**: Any upstream-merge or compatibility concerns
- **Related issues**: Link related issues with `Closes #N` or `Relates to #N`
- If core files changed: justification and `CORE_MODIFICATIONS.md` entry
- Pre-commit/test commands run and outcomes

## List Open PRs

```
mcp_github_github_list_pull_requests(owner="rivie13", repo="Phoenix-Agentic-Engine", state="open")
```

## Check PR Status

```
mcp_github_github_list_pull_requests(owner="rivie13", repo="Phoenix-Agentic-Engine", state="open")
# or use active/open PR management tools for deeper status/details
```

## PR CI/workflow gate (required)

After PR creation and after each push:

1. Check PR status checks and workflow runs.
2. If any GitHub Actions run fails, use the `github-actions-debug` skill to triage logs and root cause.
3. Fix failures in code/workflow as needed.
4. Re-run pre-commit + relevant tests locally.
5. Push fixes and re-check until required checks pass.

## Request Copilot Review

Policy:

1. Copilot review should happen automatically for new PRs.
2. Before requesting, check whether Copilot review was already requested/completed for that PR.
3. If already present, do not request again by default (cost control).
4. Request another round only when the user explicitly asks or when substantial new changes were pushed and no recent Copilot pass exists.

```
mcp_github_github_request_copilot_review(owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

## After review feedback

1. Fetch review comments/status.
2. Apply fixes in focused commits.
3. Re-run pre-commit + relevant tests.
4. Push updates to the same PR branch.
5. Request Copilot re-review only if needed and missing for the latest commit set.

## Project board field management

**Labels ≠ project fields.** Priority, Size, Work mode, Area, and Status are **project board fields** (rivie13/projects/3), NOT GitHub labels.

To set project fields, add **signal labels** (`set:<field>:<value>`) when updating an issue:

```text
mcp_github_github_issue_write(method="update", ..., labels=["task", "set:priority:p1", "set:size:m", "set:area:module-mcp"])
```

The `sync-project-fields.yml` workflow sets the field via GraphQL and removes the signal label automatically.

**Signal labels:** `set:priority:p0`–`p3`, `set:size:xs`/`s`/`m`/`l`, `set:workmode:cloud-agent`/`local-ide`/`cli-agent`, `set:status:backlog`/`ready`/`in-progress`/`in-review`/`done`, `set:area:<area-name>`.

For `cloud-agent` labeled issues, `cloud-agent-assign.yml` already handles Work mode + Status — only add priority, size, and area signal labels.

## Post-merge issue completion (mandatory)

After a PR is merged, **always** verify and close linked issues. Do NOT rely solely on `Closes #N` in the PR body — GitHub only auto-closes issues when a PR merges into the repo's **default branch**. Subfeature PRs merging into `feature/*` branches will NOT auto-close issues.

### Step 1: Close the linked issue

```
mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<N>, state="closed", stateReason="completed")
```

### Step 2: Close any completed sub-issues

Read the parent issue to list sub-issues:

```
mcp_github_github_issue_read(owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<PARENT_N>)
```

For each sub-issue whose work is merged, close it if still open:

```
mcp_github_github_issue_write(method="update", owner="rivie13", repo="<SUB_ISSUE_REPO>", issueNumber=<SUB_N>, state="closed", stateReason="completed")
```

### Step 3: Check if parent epic should be closed

If the closed issue was a sub-issue of an epic, check whether **all** sibling sub-issues are now closed. If so, close the epic:

```
mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<EPIC_N>, state="closed", stateReason="completed")
```

### Step 4: Set Status → Done on project board

Add a signal label to move the issue to Done:

```
mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<N>, labels=["task", "set:status:done"])
```

The `sync-project-fields.yml` workflow will set the Status field and remove the signal label.

> **Rule:** Never consider a PR "fully done" until all linked issues are verified closed and moved to Done. This is as important as passing CI.

## Update PR Branch (rebase/merge from base)

```
mcp_github_github_update_pull_request_branch(owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

## Push Fixes to a PR Branch

```
mcp_github_github_push_files(
  owner="rivie13",
  repo="Phoenix-Agentic-Engine",
  branch="<pr-branch>",
  message="Fix review feedback",
  files=[{path: "file.cpp", content: "..."}]
)
```

## Branch conventions

- `feature/<name>` — new features (branches from `main`, merges to `main`)
- `subfeature/<type>/<name>` — work within a feature (branches from `feature/*`, merges to `feature/*`)
  - Types: `task`, `bugfix`, `refactor`, `test`, `docs`, `chore`
- `fix/<name>` — standalone bug fixes (branches from `main`)
- `upstream-sync` — reserved for upstream Godot sync

## Issue creation (public repo — never use `gh` CLI)

- Create issues using `mcp_github_github_issue_write`.
- For non-sensitive, public-facing work: assign to Copilot (cloud agent) using `mcp_github_github_assign_copilot_to_issue`.
- Do NOT create public issues for private/sensitive matters (secrets, auth, proprietary logic, security vulnerabilities).
- Search for existing issues before creating duplicates using `mcp_github_github_search_issues`.
- Use issues to break large features into smaller, trackable units of work.

### Issue–branch alignment

- **Epic issues** (label: `epic`) map to `feature/*` branches.
- **Sub-issues** (labels: `task`, `bug`, `refactor`, `test`, `docs`, `chore`) map to `subfeature/<type>/<desc>` branches.
- Create sub-issues using `mcp_github_github_sub_issue_write`, linking them to the parent epic.
- Subfeature PRs reference sub-issues with `Closes #N`. Feature PRs reference the epic with `Closes #N`.
