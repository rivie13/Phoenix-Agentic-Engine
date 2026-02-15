---
name: pr-management
description: Create, update, and manage GitHub pull requests across Phoenix repos. Use when user asks to create a PR, update a PR description, push changes, list PRs, merge a PR, check PR status, or manage branches.
---

# PR Management — Phoenix Agentic Engine

## Repo Context

- **Owner**: `rivie13`
- **Repo**: `Phoenix-Agentic-Engine`
- **Default branch**: `main`

## Create a Pull Request

### Step 1: Check for PR template

Look for a PR template before creating:

```
mcp_github_github_get_file_contents(owner="rivie13", repo="Phoenix-Agentic-Engine", path=".github/PULL_REQUEST_TEMPLATE.md")
```

Also check `.github/PULL_REQUEST_TEMPLATE/` directory.

### Step 2: Get current branch changes

Use `get_changed_files` to see what's modified.

### Step 3: Create the PR

```
mcp_github_github_create_pull_request(
  owner="rivie13",
  repo="Phoenix-Agentic-Engine",
  title="<descriptive title>",
  body="<use template if found>",
  head="<feature-branch>",
  base="main"
)
```

### PR description should include (per repo conventions):
- What was built/changed
- What was run/tested
- Platform tested on
- If core files changed: justification and `CORE_MODIFICATIONS.md` entry

## List Open PRs

```
mcp_github_github_list_pull_requests(owner="rivie13", repo="Phoenix-Agentic-Engine", state="open")
```

## Check PR Status

```
mcp_github_github_pull_request_read(method="get", owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

## Request Copilot Review

```
mcp_github_github_request_copilot_review(owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

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
