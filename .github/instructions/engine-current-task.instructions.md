# PRIORITY — Current Task Context

## Active task pointer

**Read `.github/context/CURRENT_TASK.md` at the start of every session.** This file is the single source of truth for what is actively being worked on in this repo.

- If the file contains an active task: that is your primary context. All work should relate to or acknowledge this task.
- If the file says "No active task": use the `focus` skill to pick the next task from the roadmap.
- If you are spawned for a side task (bug fix, tangent): still read CURRENT_TASK.md to understand the main thread. Note in your response that the main task is X and this is a tangent.

## Project board

All tasks are tracked on the **Phoenix Project Board**: https://github.com/users/rivie13/projects/3

Board columns: Backlog → Ready → In Progress → In Review → Done

## Task lifecycle

1. **Pick**: Read roadmap → select next task → create/assign issue → fill CURRENT_TASK.md → move to "In Progress"
2. **Work**: Implement the task, updating checkpoint in CURRENT_TASK.md periodically
3. **Complete**: Verify acceptance criteria → move to "In Review"/"Done" → reset CURRENT_TASK.md
4. **Next**: Repeat from step 1

## Cross-repo coordination

When a task in this repo depends on or affects another repo:
- Note the dependency in CURRENT_TASK.md "Depends on" field
- Check the other repo's CURRENT_TASK.md to avoid conflicts
- Related repos: Backend (`rivie13/Phoenix-Agentic-Engine-Backend`), Interface (`rivie13/Phoenix-Agentic-Engine-Interface`)

## Privacy

This repo is **public**. CURRENT_TASK.md must not contain secrets, private strategy, or internal business details.
