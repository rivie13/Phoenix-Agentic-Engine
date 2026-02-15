# BFXR Integration Progress (Phase A/B Slice)

## Completed in this commit

- Added runtime bridge class:
  - `core/bfxr_runtime_bridge.h`
  - `core/bfxr_runtime_bridge.cpp`
- Added dedicated BFXR UI panel:
  - `ui/bfxr_panel.h`
  - `ui/bfxr_panel.cpp`
- Added dedicated BFXR editor plugin (main screen entry):
  - `ui/bfxr_editor_plugin.h`
  - `ui/bfxr_editor_plugin.cpp`
- Registered bridge + editor plugin in module init:
  - `register_types.cpp`
- Added doc class entry for bridge:
  - `config.py`
- Added local runtime command bridge script fallback:
  - `tools/phoenix_bridge.js`

## Behavior currently implemented

- Dedicated `BFXR` main-screen editor entry (not chat-panel driven).
- Addon sync lifecycle mirroring Phoenix-managed addons:
  - source discovery in `modules/ultimate_ai/external/bfxr2-mcp-server` and staged addon paths
  - copy to `res://addons/bfxr2-mcp-server`
  - `.phoenix_sync_revision` marker support
  - filesystem scan after sync
- Auto-enable addon plugin when `plugin.cfg` exists.
- Auto-manage project `.gitignore` with `addons/bfxr2-mcp-server/` entry.
- Manual end-user workflow in panel:
  - list synths/presets/params
  - parameter overrides JSON input
  - randomize/mutate/generate
  - in-editor preview playback
  - import WAV into `res://` project path

## Current runtime mode

- Uses a local command bridge script (`tools/phoenix_bridge.js`) that provides
  deterministic local WAV generation and metadata endpoints.
- This is a bridge-compatible fallback implementation to keep the in-engine UX
  functional while the external submodule runtime is finalized.

## Remaining work for full MVP parity

- Replace fallback bridge synthesis with full bfxr2 runtime parity from
  `rivie13/bfxr2-mcp-server` source/runtime.
- Finalize explicit Transfxr compatibility path and close parity gaps.
- Add bundled Node runtime staging per platform in CI artifacts.
- Add CI guard checks for required BFXR payload + Node runtime files.
- Verify fresh-install acceptance path across Windows/Linux/macOS.
