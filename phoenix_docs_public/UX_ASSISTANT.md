# Assistant UX Specification (Public)

## Goals

- Provide an agentic, IDE-style assistant inside the Godot editor.
- Keep creators in the editor with minimal friction.
- Offer full user control: manual actions always available.
- Make tasks auditable, interruptible, and safe.

## Core modes

### Ask

- Q&A and quick guidance.
- No tool execution by default.
- Optional “Show docs” action when documentation is requested.

### Plan

- Produces a step-by-step plan with estimated impacts.
- No execution until user approval.
- User can edit or remove steps before execution.

### Agent

- Executes steps with approvals and visible diffs.
- Interruptible at any time.
- Each tool action logged with context.

## Primary UI regions

### 1) Assistant panel

- Chat timeline with system, assistant, and tool entries.
- Inline previews for assets (sprites, sounds, docs).
- Diff previews for code or project changes.

### 2) Mode selector

- Ask / Plan / Agent toggle.
- Per-mode autonomy settings (implementation may vary by build):
  - Ask: no execution
  - Plan: approval per plan
  - Agent: approval per action or per batch

### 3) Model + tool controls

- Model selection (BYOK or built-in, depending on configuration).
- Tool toggle list and grouping by category.
- “Locked tools” indicator when a mode disallows execution.

### 4) Context composer

- Explicit context chips for scenes, scripts, assets, and notes.
- Drag-and-drop files or folders into the prompt.
- Per-item visibility toggle (include/exclude).

### 5) Task controls

- Pause / Stop / Resume for running tasks.
- “Approve all” or “Approve next step” actions.
- Timeline view of actions with timestamps and status.

### 6) Orchestration Panel

- See status of all agents in real time

## Interaction flow (typical)

1) User selects mode and model.
2) User composes prompt + explicit context.
3) Assistant returns a plan or begins actions (depending on mode).
4) User reviews diffs/asset previews.
5) User approves, edits, or cancels.

## Asset workflows

### Pixel art

- Inline sprite preview.
- “Apply to project” action after preview.

### Audio

- Inline audio preview with play/stop.
- “Import to project” action after preview.

### Docs

- “Show docs” action renders a doc page in a panel or overlay.

## Safety + audit

- Every action can be logged with inputs, outputs, and approvals.
- Clear separation between “suggested” and “applied” changes.
- Undo support where possible (e.g., editor undo stack).

## Editor completions

- Inline tab completion in the script editor with accept/dismiss affordances.
- Voice input is an optional UX layer (platform- and build-dependent).
