---
name: github-code-review
description: Fetch and address GitHub pull request code review comments, including Copilot code reviews. Use when user asks to get review feedback, address review comments, fix review issues, request a code review, check PR reviews, or respond to reviewer feedback.
---

# GitHub Code Review — Phoenix Agentic Engine

## Repo Context

- **Owner**: `rivie13`
- **Repo**: `Phoenix-Agentic-Engine`
- **Type**: Public (Godot Engine fork)

## Workflow: Fetch & Address Review Comments

### Step 1: Identify the PR

If the user doesn't specify a PR number, list open PRs:

```
mcp_github_github_list_pull_requests(owner="rivie13", repo="Phoenix-Agentic-Engine", state="open")
```

### Step 2: Get review comments

Fetch review threads (which include resolved/unresolved status):

```
mcp_github_github_pull_request_read(method="get_review_comments", owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

Also get the list of reviews to understand overall review status:

```
mcp_github_github_pull_request_read(method="get_reviews", owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

### Step 3: Get changed files for context

```
mcp_github_github_pull_request_read(method="get_files", owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

### Step 4: Address each unresolved comment

For each unresolved review thread:
1. Read the file and surrounding context using `read_file`
2. Understand the reviewer's concern
3. Make the fix using file edit tools
4. Report what was changed and why

### Step 5: Request a new review (optional)

After addressing all comments, the user may want to request a re-review:

```
mcp_github_github_request_copilot_review(owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

## Workflow: Request a Copilot Code Review

To request an automated Copilot code review on a PR:

```
mcp_github_github_request_copilot_review(owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

## Review priorities for this repo

When addressing reviews, keep in mind the Engine repo review priorities:
- Changes in `modules/ultimate_ai/` are low-risk and expected
- Changes in `core/`, `scene/`, `servers/`, `editor/` are high-risk — require justification
- Any code outside `modules/ultimate_ai/` needs a `CORE_MODIFICATIONS.md` entry
- Match Godot coding style (tabs, snake_case, PascalCase classes)
- Editor-only code must be guarded by `#ifdef TOOLS_ENABLED`
- No secrets, credentials, or prompt content
