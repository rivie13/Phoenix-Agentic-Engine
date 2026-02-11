#!/usr/bin/env bash
set -euo pipefail

PLATFORM="${1:-linuxbsd}"
ENGINE_ROOT="${2:-"$(cd "$(dirname "$0")/../.." && pwd)"}"
API_FILE_OVERRIDE="${3:-""}"
KEEP_BUILD="${KEEP_BUILD:-0}"

API_FILE="$ENGINE_ROOT/extension_api.json"
if [ -n "$API_FILE_OVERRIDE" ]; then
  API_FILE="$API_FILE_OVERRIDE"
fi
API_FILE_DIR="$(cd "$(dirname "$API_FILE")" && pwd)"
API_FILE="$API_FILE_DIR/$(basename "$API_FILE")"
if [ ! -f "$API_FILE" ]; then
  echo "Missing extension_api.json at $API_FILE" >&2
  exit 1
fi
GODOT_CPP_BRANCH=$(API_FILE="$API_FILE" python3 - <<'PY'
import json
import os
api_file = os.environ.get("API_FILE")
if not api_file:
    raise SystemExit("API_FILE env var not set")
with open(api_file, "r", encoding="utf-8") as fh:
    data = json.load(fh)
header = data.get("header", {})
major = header.get("version_major")
minor = header.get("version_minor")
status = header.get("version_status")
if status == "stable":
  print(f"godot-{major}.{minor}-stable")
else:
  print("master")
PY
)

if ! command -v perl >/dev/null 2>&1; then
  echo "Missing perl in PATH. Install perl and retry." >&2
  exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "Missing cmake in PATH. Install CMake and retry." >&2
  exit 1
fi

BUILD_ROOT="${RUNNER_TEMP:-/tmp}/phoenix-git-plugin-build"
rm -rf "$BUILD_ROOT"

if [ "$PLATFORM" = "linuxbsd" ]; then
  PLATFORM="linux"
fi

git clone --depth 1 https://github.com/rivie13/godot-git-plugin.git "$BUILD_ROOT"

pushd "$BUILD_ROOT" >/dev/null
git submodule update --init --recursive --depth 1
pushd "$BUILD_ROOT/godot-cpp" >/dev/null
git fetch origin
if git show-ref --verify --quiet "refs/remotes/origin/$GODOT_CPP_BRANCH"; then
  git checkout "$GODOT_CPP_BRANCH"
else
  git checkout master
fi
popd >/dev/null
python3 -m SCons platform="$PLATFORM" target=editor dev_build=yes generate_bindings=yes custom_api_file="$API_FILE"
popd >/dev/null

DST="$ENGINE_ROOT/bin/addons/godot-git-plugin"
rm -rf "$DST"
mkdir -p "$DST"
cp -R "$BUILD_ROOT/addons/godot-git-plugin/." "$DST"

if [ "$KEEP_BUILD" != "1" ]; then
  rm -rf "$BUILD_ROOT"
fi
