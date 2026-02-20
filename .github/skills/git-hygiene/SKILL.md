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

1. Sync base branch:

```bash
git checkout feature/agent-backend-integration   # or main
git pull --rebase origin feature/agent-backend-integration
```

2. Create focused branch:

```bash
git checkout -b feature/<short-topic>
```

3. Keep PR scope single-purpose and small.

## PR size discipline (mandatory)

- Keep PRs small and focused — one logical change per PR.
- If a feature branch grows large, break it into sub-branches:
  1. Create sub-branches off the feature branch for discrete pieces of work.
  2. Open PRs from each sub-branch into the feature branch.
  3. Once sub-branch PRs are merged into the feature branch, open a single PR from the feature branch into `main`.
- Target: PRs should ideally be under ~400 lines of meaningful change (excluding generated files, lock files, submodule pointer updates).
- If a PR exceeds this, strongly consider splitting before requesting review.

## Pre-commit quality gate (required)

Run all:

```powershell
C:\Python313\python.exe -m pre_commit run --all-files
C:\Python313\python.exe -m SCons platform=windows target=editor d3d12=no
```

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

## Issue creation (public repo rules)

- Create issues using `mcp_github_github_issue_write` — never `gh issue create`.
- For non-sensitive, public-facing work: assign to Copilot (cloud agent) using `mcp_github_github_assign_copilot_to_issue`.
- Do NOT create public issues for private/sensitive matters (secrets, auth, proprietary logic, infrastructure, security vulnerabilities).
- Search for existing issues before creating duplicates using `mcp_github_github_search_issues`.

## Engine-specific checks for PR readiness

- Pre-commit passes
- SCons build succeeds
- PR workflow/status checks are green
- Changes outside `modules/ultimate_ai/` have `CORE_MODIFICATIONS.md` entry
- No secrets, credentials, or prompt content
