#!/usr/bin/env bash
set -euo pipefail

PLATFORM="${1:-linux}"
ENGINE_ROOT="${2:-"$(cd "$(dirname "$0")/../.." && pwd)"}"

SOURCE_DIR="${ENGINE_ROOT}/modules/ultimate_ai/external/gut/addons/gut"
DEST_DIR="${ENGINE_ROOT}/bin/addons/gut"

if [ ! -d "$SOURCE_DIR" ]; then
  echo "[gut] Source path not found: $SOURCE_DIR" >&2
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

if [ ! -f "$DEST_DIR/plugin.cfg" ]; then
  echo "[gut] Missing staged plugin.cfg in $DEST_DIR" >&2
  exit 1
fi

if [ ! -f "$DEST_DIR/gut_plugin.gd" ]; then
  echo "[gut] Missing staged gut_plugin.gd in $DEST_DIR" >&2
  exit 1
fi

echo "[gut] Staged addon payload for platform: $PLATFORM"
