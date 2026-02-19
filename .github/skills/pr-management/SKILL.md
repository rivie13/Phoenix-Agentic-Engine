---
name: pr-management
description: Create, update, and manage GitHub pull requests across Phoenix repos. Use when user asks to create a PR, update a PR description, push changes, list PRs, merge a PR, check PR status, or manage branches.
---

# PR Management — Phoenix Agentic Engine

## Repo Context

- **Owner**: `rivie13`
- **Repo**: `Phoenix-Agentic-Engine`
- **Canonical integration base**: `feature/agent-backend-integration` (unless user specifies another base)

## Branch hygiene (required)

1. Start from the latest target base branch.
2. Create a focused topic branch (`feature/*`, `fix/*`, `mcp-docs/*`, etc.).
3. Keep PR scope narrow; avoid unrelated file changes.
4. Run pre-commit + relevant tests before push.
5. Never force-push shared branches unless explicitly coordinated.

Example setup:

```bash
git checkout feature/agent-backend-integration
git pull --rebase origin feature/agent-backend-integration
git checkout -b <topic-branch>
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
  head="<feature-branch>",
  base="feature/agent-backend-integration"
)
```

### PR description should include (per repo conventions):
- What was built/changed
- What was run/tested
- Platform tested on
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
5. Request Copilot re-review if needed.

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

- `feature/<name>` — new features
- `fix/<name>` — bug fixes
- `upstream-sync` — reserved for upstream Godot sync
