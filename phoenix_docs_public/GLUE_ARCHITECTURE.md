# Glue Architecture (Public)

Phoenix’s “glue layer” describes how the editor UI, local tools, and optional external tools connect into one workflow.

## High-level layers

1) **Assistant UI layer**
   - Chat timeline
   - Ask/Plan/Agent mode selection
   - Context composer
   - Approval controls and previews

2) **Local orchestration (editor-side)**
   - Routes user intent to the right capability
   - Manages context attachments
   - Tracks task state (running/paused/stopped)

3) **Tool adapters**
   - Editor tools (scene edits, script edits, resource updates)
   - Asset tools (e.g., pixel art, audio) where available
   - VCS tools (diff preview, staged changes)

4) **External tool bridge (optional)**
   - Connects to external tool servers (docs, project management, etc.)
   - Provides discovery and invocation of tools

## Process boundaries

External tools often run out-of-process. A good glue layer:

- keeps the UI responsive
- shows tool inputs/outputs in the timeline
- enforces permissions and user approvals
- isolates failures (one tool crashing shouldn’t break the editor)

## Why this matters

The goal is a consistent user experience:

- the assistant can *suggest* and *preview* changes
- users can approve or reject
- the editor applies changes deterministically
