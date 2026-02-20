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

- Always branch from the target base (usually `feature/agent-backend-integration` or `main`).
- Keep one focused concern per branch/PR.
- Branch naming:
  - `feature/<short-topic>`
  - `fix/<short-topic>`
  - `mcp-docs/<short-topic>`
  - `chore/<short-topic>` for non-functional maintenance
- Avoid direct commits to shared integration branches.
- Rebase/sync regularly with the base branch to reduce merge drift.

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
- If a feature branch grows large, break it into sub-branches:
  1. Create sub-branches off the feature branch for discrete pieces of work.
  2. Open PRs from each sub-branch into the feature branch.
  3. Once sub-branch PRs are merged into the feature branch, open a single PR from the feature branch into `main`.
- Target: PRs should ideally be under ~400 lines of meaningful change (excluding generated files, lock files, submodule pointer updates).
- If a PR exceeds this, strongly consider splitting before requesting review.
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

## Engine quality gate (required before PR readiness)

- `pre_commit run --all-files` passes
- SCons build succeeds
- PR GitHub Actions checks are green (or explicitly understood/waived)
- Changes in `modules/ultimate_ai/` are expected; changes outside require `CORE_MODIFICATIONS.md` entry
- No secrets, credentials, or prompt content committed
