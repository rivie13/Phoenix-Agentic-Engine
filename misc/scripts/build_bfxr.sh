#!/usr/bin/env bash
set -euo pipefail

PLATFORM="${1:-linux}"
ENGINE_ROOT="${2:-"$(cd "$(dirname "$0")/../.." && pwd)"}"

SOURCE_DIR="${ENGINE_ROOT}/modules/ultimate_ai/external/bfxr2-mcp-server"
DEST_DIR="${ENGINE_ROOT}/bin/addons/bfxr2-mcp-server"
BRIDGE_SOURCE="${ENGINE_ROOT}/modules/ultimate_ai/tools/phoenix_bridge.js"

if [ ! -d "$SOURCE_DIR" ]; then
  echo "[bfxr] Source path not found: $SOURCE_DIR" >&2
  exit 1
fi

if [ ! -f "$BRIDGE_SOURCE" ]; then
  echo "[bfxr] Missing bridge script: $BRIDGE_SOURCE" >&2
  exit 1
fi

mkdir -p "${ENGINE_ROOT}/bin/addons"
rm -rf "$DEST_DIR"
mkdir -p "$DEST_DIR"

if command -v rsync >/dev/null 2>&1; then
  rsync -a --delete \
    --exclude='.git' \
    --exclude='.github' \
    --exclude='.vscode' \
    --exclude='.DS_Store' \
    --exclude='.cursorignore.txt' \
    "$SOURCE_DIR/" "$DEST_DIR/"
else
  cp -R "$SOURCE_DIR/." "$DEST_DIR/"
  rm -rf "$DEST_DIR/.git" "$DEST_DIR/.github" "$DEST_DIR/.vscode"
  rm -f "$DEST_DIR/.DS_Store" "$DEST_DIR/.cursorignore.txt"
fi

cp "$BRIDGE_SOURCE" "$DEST_DIR/phoenix_bridge.js"

if [ ! -f "$DEST_DIR/mcp-server.js" ]; then
  echo "[bfxr] Missing staged mcp-server.js in $DEST_DIR" >&2
  exit 1
fi

if [ ! -f "$DEST_DIR/package.json" ]; then
  echo "[bfxr] Missing staged package.json in $DEST_DIR" >&2
  exit 1
fi

if [ ! -f "$DEST_DIR/phoenix_bridge.js" ]; then
  echo "[bfxr] Missing staged phoenix_bridge.js in $DEST_DIR" >&2
  exit 1
fi

echo "[bfxr] Staged runtime payload for platform: $PLATFORM"
