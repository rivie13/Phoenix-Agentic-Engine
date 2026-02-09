# Core Modifications Tracker

## Files Modified Outside Module

### editor/icons/DefaultProjectIcon.svg

- **Replaced** the upstream Godot icon SVG (PNG-in-SVG using `<image>` tag) with an SVGO-optimized version of `branding/Phoenix_app_icon.svg` (pure vector `<path>` elements).
- **Why**: ThorVG does not support `<image>` tags with embedded PNG data URIs, producing a 0×0 pixel output and triggering `ERROR: Invalid image: image is empty` / `ERROR: Image width must be greater than 0.` errors. The replacement is a proper vector SVG (~92 KB) that ThorVG renders correctly.
- **Merge note**: Upstream changes to this file can be ignored — always keep our Phoenix SVG.

### editor/editor_node.cpp

- TODO: Add #include for assistant panel
- TODO: Register the Ultimate AI editor plugin in _init_plugins()

### editor/SCsub

- TODO: Add module include path (if needed)

## Why These Are Necessary

- Editor plugin system requires registration in editor_node.cpp
- No other way to add main editor UI panels

## Merge Conflict Resolution Notes

- These lines should sit near other plugin registrations
- If conflict: accept upstream changes, then re-apply these edits
