# Architecture Overview (Public)

Phoenix Agentic Engine extends the Godot editor with an agentic assistant workflow while keeping the underlying engine aligned with upstream Godot.

## The three-repo model

Phoenix is structured as multiple repositories with clear separation of concerns:

- **Engine (this repo)**: editor UX and local execution hooks (panels, previews, approvals, adapters).
- **Interface SDK**: versioned protocol contracts and a typed client SDK that defines the public boundary between editor and any backend.
- **Backend service (optional / proprietary)**: orchestration, multi-agent coordination, policy, and other server-side logic.

The open-source pieces are designed to remain useful even without a backend.

## “Brain / body” split

A useful way to think about the architecture:

- **Body (editor/runtime)**: the Godot fork and editor plugins. This is where UI, previews, and local tool execution live.
- **Nervous system (interface contracts)**: the typed protocol that carries requests, context, and results.
- **Brain (optional backend)**: plans and coordinates multi-step work, routes requests to models/tools, and returns actions/results.

## Core concepts

### Ask / Plan / Agent

- **Ask**: quick answers and suggestions.
- **Plan**: a reviewable step list before any execution.
- **Agent**: executes actions with user-visible previews/diffs and approval controls.

### Explicit context

Prompts are paired with explicit context attachments (scenes, scripts, assets, notes) so users can see what is included and control it.

### Tools via adapters

Editor actions and external tools are accessed through adapters so the assistant can:

- preview outputs before applying changes
- apply changes in a deterministic way
- log actions for auditability

### Scene snapshots (high level)

For deeper editor understanding, the editor can produce a structured snapshot of scene state (and incremental updates) so planning/execution can reason about a project as more than just files.

## Typical data flow (high level)

1) User writes a request and selects mode.
2) The editor assembles explicit context.
3) A plan is produced (Plan mode) or actions begin (Agent mode).
4) Proposed changes are shown as previews/diffs.
5) User approves; the editor applies changes locally.
6) Results are summarized in the assistant timeline.

## Design constraints

- Phoenix aims to keep core engine changes minimal.
- The editor must remain usable without agent features.
- Users retain control: stop/pause and approvals are first-class.
