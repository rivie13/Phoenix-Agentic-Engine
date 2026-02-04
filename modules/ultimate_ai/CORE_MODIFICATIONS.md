# Core Modifications Tracker

## Files Modified Outside Module

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
