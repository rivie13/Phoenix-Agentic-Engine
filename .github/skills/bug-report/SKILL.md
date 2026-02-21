---
name: bug-report
description: Investigate a bug and create a high-quality GitHub bug issue with reproducible steps and code-level findings. Use when user asks to report a bug, file an issue, document a regression, or write an investigation-backed bug report.
---

# Bug report — Phoenix Agentic Engine

## Mandatory first step

Read the active task pointer so bug work is framed as primary work or tangent:

```
read_file(".github/context/CURRENT_TASK.md")
```

## Workflow

1. Clarify bug scope
   - Capture expected behavior, actual behavior, and impact.
   - If context is missing, ask only the minimum needed to make the report actionable.

2. Gather investigation evidence
   - Inspect relevant code paths, logs, and contracts.
   - Capture suspected root-cause area and what was ruled out.
   - Prefer concrete file/function references.

3. Check for duplicates
   - Use GitHub MCP issue search before creating a new issue:
   ```
   mcp_github_github_search_issues(owner="rivie13", repo="Phoenix-Agentic-Engine", query="<keywords>")
   ```

4. Create the issue with the bug template structure
   - Use `mcp_github_github_issue_write` (`method="create"`).
   - Add `bug` label.
   - Include: summary, environment, repro steps, expected vs actual, evidence, investigation notes, and acceptance criteria.

5. Set project fields via signal labels (when available)
   - Add labels such as `set:priority:p1`, `set:size:m`, `set:workmode:local-ide`.
   - If the task should be cloud-run, move it to Ready and add `cloud-agent`.

## Required issue quality bar

- Reproduction is deterministic (or clearly states flakiness conditions).
- Investigation contains at least one concrete lead (not just symptoms).
- Fix acceptance criteria are testable and specific.
- Scope is one bug per issue.

## Related repos

- Backend: `rivie13/Phoenix-Agentic-Engine-Backend`
- Interface: `rivie13/Phoenix-Agentic-Engine-Interface`

Use cross-repo links in the issue body when the bug spans boundaries.
