---
name: git-hygiene
description: Enforce branch hygiene, pre-commit validation, PR setup via GitHub MCP tools, and review follow-up workflow in the Engine repo. Use when user asks about git workflow, PR creation, branch management, or commit hygiene.
---

# Git Hygiene — Phoenix Agentic Engine

## CLI tool policy (mandatory)

- **NEVER use `gh` CLI** — it is not installed and must not be used.
- **Always prefer GitHub MCP tools** (`mcp_github_*`) for all GitHub operations.
- Fall back to terminal `git` commands only for local worktree operations or when MCP tools fail.
- Do NOT suggest or attempt any `gh` subcommand.

## Mandatory first step: terminal scope check

1. `Set-Location "C:\Users\rivie\vsCodeProjects\Phoenix-Agentic-Engine"`
2. `Get-Location`
3. `git rev-parse --show-toplevel`
4. `git branch --show-current`

## Branch workflow

### Hierarchy

| Tier | Pattern | Branches from | Merges into |
|------|---------|---------------|-------------|
| **main** | `main` | — | — |
| **feature** | `feature/<topic>` | `main` | `main` |
| **subfeature** | `subfeature/<type>/<desc>` | parent `feature/*` | parent `feature/*` |

### Starting a feature

```bash
git checkout main
git pull --rebase origin main
git checkout -b feature/<topic>
```

### Starting a subfeature (daily work)

```bash
git checkout feature/<topic>
git pull --rebase origin feature/<topic>
git checkout -b subfeature/<type>/<short-description>
```

Where `<type>` is one of: `task`, `bugfix`, `refactor`, `test`, `docs`, `chore`.

**Examples:**
- `subfeature/task/wire-up-assistant-panel`
- `subfeature/bugfix/fix-panel-null-crash`
- `subfeature/refactor/extract-message-renderer`

### Subfeature PRs target the feature branch — never `main`

```bash
# PR: subfeature/task/wire-up-assistant-panel → feature/assistant-panel
```

### Feature PRs target `main` (when all subfeatures are merged)

```bash
# PR: feature/assistant-panel → main   (will be large — that is expected)
```

### Top-level branches (non-feature work)

For small standalone changes: `fix/<topic>`, `chore/<topic>`, `mcp-docs/<topic>` — branch from and merge to `main`.

3. Keep PR scope single-purpose and small.

## PR size discipline (mandatory)

- Keep PRs small and focused — one logical change per PR.
- **Subfeature → feature PRs** are the normal unit of review.
- **Feature → main PRs** will be large — that is expected.
- Break work into subfeature branches early:
  1. Create `subfeature/<type>/<description>` branches off the parent `feature/*` branch.
  2. Open PRs from each subfeature branch into the `feature/*` branch.
  3. Once all subfeature PRs are merged, open a single PR from `feature/*` into `main`.
- Target: subfeature PRs should ideally be under ~400 lines of meaningful change (excluding generated files, lock files, submodule pointer updates).
- If a subfeature PR exceeds this, strongly consider splitting before requesting review.

## Pre-commit quality gate (required)

Run all:

```powershell
C:\Python313\python.exe -m pre_commit run --all-files
C:\Python313\python.exe -m SCons platform=windows target=editor d3d12=no
```

Preferred VS Code task for pre-commit:

- `dev: precommit: check`
   - Empty prompt = `--all-files` (default).
   - Space-separated files = `--files ...`.
   - Full-repo runs are long-running; keep focus on this task and wait until it fully completes before commit/PR steps.

If any command fails, fix before committing.

## Commit workflow

```bash
git status
git add <focused file set>
git commit -m "feat: <short summary>"
```

Recommended prefixes: `feat`, `fix`, `chore`, `docs`, `test`.

## PR workflow (use GitHub MCP tools — never `gh` CLI)

1. Push branch from terminal:

```bash
git push -u origin <feature-branch>
```

2. Create PR via MCP tool:

```text
mcp_github_github_create_pull_request(owner="rivie13", repo="Phoenix-Agentic-Engine", title="...", body="<detailed description>", head="<branch>", base="feature/agent-backend-integration")
```

3. Check whether Copilot review already exists for the latest commits:

```text
mcp_github_github_pull_request_read(method="get_reviews", owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

If Copilot review is missing for the latest commit set, request it:

```text
mcp_github_github_request_copilot_review(owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

4. Read and handle reviews/comments:

```text
mcp_github_github_pull_request_read(method="get_reviews", owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
mcp_github_github_pull_request_read(method="get_review_comments", owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

5. Check PR workflow status and fix failures before merge readiness:

```text
mcp_github_github_actions_list(method="list_workflow_runs", owner="rivie13", repo="Phoenix-Agentic-Engine")
mcp_github_github_actions_list(method="list_workflow_jobs", owner="rivie13", repo="Phoenix-Agentic-Engine", resource_id="<RUN_ID>")
```

If any job fails, use the GitHub Actions Debug skill flow to inspect logs, fix root causes, and re-run/recheck.

6. Apply fixes and re-run quality gate.

## Post-merge issue completion (mandatory)

After any PR is merged, **always close linked issues explicitly**. Do NOT rely solely on `Closes #N` in the PR body — GitHub only auto-closes issues when merging into the repo's **default branch**. Subfeature PRs that merge into `feature/*` branches will NOT auto-close linked issues.

1. **Close the linked issue:**
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<N>, state="closed", stateReason="completed")
   ```

2. **Close completed sub-issues** — if this was a parent issue with sub-issues, verify each merged sub-issue is closed.

3. **Close parent epic if all children are done** — if this was a sub-issue, check whether all sibling sub-issues are now closed. If so, close the parent epic too.

4. **Set Status → Done** on the project board using a signal label:
   ```
   mcp_github_github_issue_write(method="update", owner="rivie13", repo="Phoenix-Agentic-Engine", issueNumber=<N>, labels=["task", "set:status:done"])
   ```

> **Rule:** A PR is not "fully done" until all linked issues are verified closed.

## Issue creation (public repo rules)

- Create issues using `mcp_github_github_issue_write` — never `gh issue create`.
- For non-sensitive, public-facing work: assign to Copilot (cloud agent) using `mcp_github_github_assign_copilot_to_issue`.
- Do NOT create public issues for private/sensitive matters (secrets, auth, proprietary logic, infrastructure, security vulnerabilities).
- Search for existing issues before creating duplicates using `mcp_github_github_search_issues`.

### Issue–branch alignment

- **Epic issues** (label: `epic`) map to `feature/*` branches.
- **Sub-issues** (labels: `task`, `bug`, `refactor`, `test`, `docs`, `chore`) map to `subfeature/<type>/<desc>` branches.
- Create sub-issues using `mcp_github_github_sub_issue_write`, linking them to the parent epic.
- Subfeature PRs reference sub-issues with `Closes #N`. Feature PRs reference the epic with `Closes #N`.

## Engine-specific checks for PR readiness

- Pre-commit passes
- SCons build succeeds
- PR workflow/status checks are green
- Changes outside `modules/ultimate_ai/` have `CORE_MODIFICATIONS.md` entry
- No secrets, credentials, or prompt content
