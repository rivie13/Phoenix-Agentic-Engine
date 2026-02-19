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

### Step 2: Load PR details/comments with MCP PR tools

Use PR management tools to inspect the active/open PR (files, reviews, comments, checks):

1. Activate PR management tools.
2. Retrieve the active/open PR details and comments.
3. Use those comments as the canonical fix list.

### Step 3: Address each unresolved comment

For each unresolved review thread:
1. Read the file and surrounding context using `read_file`
2. Understand the reviewer's concern
3. Make the fix using file edit tools
4. Re-run pre-commit + relevant tests
5. Report what was changed and why

### Step 3b: Check PR workflows and fix CI failures

After pushing review fixes:

1. Check PR status checks and workflow runs.
2. If any workflow fails, use the `github-actions-debug` skill to investigate logs.
3. Apply fixes, re-run local validation, and push again.
4. Repeat until required checks are green.

### Step 4: Request a new review (optional)

After addressing all comments, the user may want to request a re-review:

- First check if a Copilot review is already present/recent on the PR.
- Only request if none exists for the latest commit set, or if the user explicitly asks for another pass.

```
mcp_github_github_request_copilot_review(owner="rivie13", repo="Phoenix-Agentic-Engine", pullNumber=<PR_NUMBER>)
```

## Copilot review resolution loop (required)

- Treat Copilot comments like any other review: fix, validate, push.
- Prefer small, focused commits per feedback batch.
- Run `pre_commit` and relevant tests before pushing each fix batch.
- If a comment is not actioned, reply with rationale in the PR.

## Workflow: Request a Copilot Code Review (manual fallback)

If a Copilot review is missing for the latest commit set on a PR:

- Copilot review is expected to auto-trigger when available.
- Do not request manually if a Copilot review already exists for the latest commit set.
- Request manually only when missing for the latest commit set.

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
