---
excludeAgent: "coding-agent"
---

# Copilot code review instructions (Engine repo)

You are reviewing PRs for a Godot fork with Phoenix editor integrations.
Priorities: minimal diffs, low upstream-merge risk, deterministic build/package behavior.

## Review focus

- Prefer changes in `modules/ultimate_ai/` and addon build scripts (`misc/scripts/build_*`, workflow addon staging).
- Treat edits under `core/`, `scene/`, `servers/`, `editor/`, `platform/`, `drivers/` as high risk.
- Require `modules/ultimate_ai/CORE_MODIFICATIONS.md` updates for Phoenix-driven edits outside `modules/ultimate_ai/`.

## Current integration surface to protect

- Module init (`modules/ultimate_ai/register_types.cpp`) should keep editor-only registration coherent:
  - `UltimateAIEditorPlugin`, `BfxrEditorPlugin`, `DiffMarginEditorPlugin`, `GDTermEditorPlugin`, `GitPluginEditorPlugin`, `GutEditorPlugin`, `PixelPenEditorPlugin`
  - `BfxrRuntimeBridge`, `UltimateAIBackendContractAdapter`, `UltimateAITerminalBridge`
- Addon payload staging must remain valid for CI/editor artifacts (`bin/addons/*`, bundled Node runtime for BFXR).

## Safety / correctness checks

- Enforce Godot style and memory/ownership safety (`memnew`/`memdelete`, `Ref<T>`, no exceptions).
- Verify editor-only logic remains under editor guards and editor-only init level assumptions.
- Flag regressions in addon bootstrap paths, sync markers, and auto-enable behavior.

## Scope hygiene

- Keep PRs narrow; split unrelated refactors.
- Avoid direct edits inside vendored third-party trees under `modules/ultimate_ai/external/*` unless intentionally updating submodule pointers.
- Reject prompt templates, secrets, credentials, or backend-only proprietary logic in public client code.

## CLI tool policy (mandatory)

- **NEVER use `gh` CLI** — it is not installed and must not be used.
- **Always prefer GitHub MCP tools** (`mcp_github_*`) for all GitHub operations (PRs, issues, reviews, actions, branches, searches).
- Fall back to terminal `git` commands only for local worktree operations or when MCP tools fail.
- Do NOT suggest or attempt any `gh` subcommand.

## PR workflow hygiene

- **Always use GitHub MCP tools** for PR operations — never `gh` CLI.
- Treat Copilot review comments as actionable items: fix or respond with rationale.
- Copilot review requests are automatic but budgeted: check whether a Copilot review is already requested/completed on the PR before requesting.
- Do not request duplicate Copilot reviews by default; request at most one per PR unless user explicitly asks for another round after substantial new changes.
- Require re-validation (pre-commit + relevant tests/build checks) before marking review feedback as addressed.
- Check PR status checks and GitHub Actions workflow runs; if failures exist, debug and fix before approval/merge.
- Verify branch/base targets are correct before merge (typically into `feature/agent-backend-integration` unless explicitly overridden).

## PR size discipline

- Keep PRs small and focused — one logical change per PR.
- If a feature branch grows large, break it into sub-branches targeting the feature branch, then merge the feature branch into `main`.
- Target: PRs should ideally be under ~400 lines of meaningful change.
- Flag oversized PRs and request splitting.

## Issue creation (public repo)

- Create issues for trackable work using GitHub MCP tools — never `gh issue create`.
- For non-sensitive, public-facing work: assign to Copilot (cloud agent) using `mcp_github_github_assign_copilot_to_issue`.
- Do NOT create public issues for private/sensitive matters (secrets, auth, proprietary logic, security vulnerabilities).
