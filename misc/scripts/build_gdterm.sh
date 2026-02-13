#!/usr/bin/env bash
set -euo pipefail

PLATFORM="${1:-linuxbsd}"
ENGINE_ROOT="${2:-"$(cd "$(dirname "$0")/../.." && pwd)"}"
API_FILE_OVERRIDE="${3:-""}"
KEEP_BUILD="${KEEP_BUILD:-0}"
REUSE_BUILD="${REUSE_BUILD:-0}"
FORCE_CLEAN="${FORCE_CLEAN:-0}"
SCONS_CACHE_PATH="${SCONS_CACHE_PATH:-${SCONS_CACHE:-}}"

if [ "$PLATFORM" = "linuxbsd" ]; then
  PLATFORM="linux"
fi

if command -v python3 >/dev/null 2>&1; then
  PYTHON_BIN="python3"
elif command -v python >/dev/null 2>&1; then
  PYTHON_BIN="python"
else
  echo "Python executable not found in PATH (expected 'python3' or 'python')." >&2
  exit 1
fi

API_FILE="$ENGINE_ROOT/extension_api.json"
if [ -n "$API_FILE_OVERRIDE" ]; then
  API_FILE="$API_FILE_OVERRIDE"
fi
API_FILE="$($PYTHON_BIN -c 'import os,sys; print(os.path.realpath(sys.argv[1]))' "$API_FILE")"
if [ ! -f "$API_FILE" ]; then
  echo "Missing extension_api.json at $API_FILE" >&2
  exit 1
fi

GODOT_CPP_BRANCH=$(API_FILE="$API_FILE" $PYTHON_BIN - <<'PY'
import json
import os

with open(os.environ["API_FILE"], "r", encoding="utf-8") as fh:
    api = json.load(fh)
header = api.get("header", {})
major = header.get("version_major")
minor = header.get("version_minor")
status = header.get("version_status")
if status == "stable":
    print(f"godot-{major}.{minor}-stable")
else:
    print(f"{major}.{minor}")
PY
)

BUILD_ROOT="${RUNNER_TEMP:-/tmp}/phoenix-gdterm-build"

reuse_effective=0
if [ "$REUSE_BUILD" = "1" ] || [ "$FORCE_CLEAN" != "1" ]; then
  reuse_effective=1
fi

if [ -d "$BUILD_ROOT" ]; then
  if [ "$reuse_effective" = "1" ]; then
    if [ ! -d "$BUILD_ROOT/.git" ]; then
      rm -rf "$BUILD_ROOT"
    fi
  else
    rm -rf "$BUILD_ROOT"
  fi
fi

if [ ! -d "$BUILD_ROOT" ]; then
  git clone --depth 1 https://github.com/rivie13/gdterm.git "$BUILD_ROOT"
fi

pushd "$BUILD_ROOT" >/dev/null
git fetch --depth 1 origin main
git reset --hard origin/main

LOCAL_SUBMODULE_PATH="$ENGINE_ROOT/modules/ultimate_ai/external/gdterm"
if [ -d "$LOCAL_SUBMODULE_PATH/.git" ]; then
  local_sha="$(git -C "$LOCAL_SUBMODULE_PATH" rev-parse HEAD 2>/dev/null || true)"
  if [ -n "$local_sha" ]; then
    git fetch --depth 1 origin "$local_sha"
    git checkout --detach "$local_sha"
  fi
fi

git submodule update --init --depth 1 godot-cpp src/gdterm/pty/thirdparty/libtmt

pushd "$BUILD_ROOT/godot-cpp" >/dev/null
selected_ref="$GODOT_CPP_BRANCH"
git fetch --depth 1 origin "$selected_ref" || selected_ref="master"
git fetch --depth 1 origin "$selected_ref"
git checkout -B "$selected_ref" "origin/$selected_ref"
echo "[gdterm] godot-cpp branch: $selected_ref"
echo "[gdterm] godot-cpp commit: $(git rev-parse HEAD)"
popd >/dev/null

mkdir -p "$BUILD_ROOT/godot-cpp/gdextension"
cp -f "$API_FILE" "$BUILD_ROOT/extension_api.json"
cp -f "$API_FILE" "$BUILD_ROOT/godot-cpp/gdextension/extension_api.json"

$PYTHON_BIN "$ENGINE_ROOT/misc/scripts/sanitize_extension_api.py" "$BUILD_ROOT/extension_api.json"
$PYTHON_BIN "$ENGINE_ROOT/misc/scripts/sanitize_extension_api.py" "$BUILD_ROOT/godot-cpp/gdextension/extension_api.json"

scons_args=(
  "platform=$PLATFORM"
  "generate_bindings=yes"
  "custom_api_file=$BUILD_ROOT/extension_api.json"
)
if [ "$PLATFORM" = "windows" ] || [ "$PLATFORM" = "linux" ]; then
  scons_args+=("arch=x86_64")
fi
if [ -n "$SCONS_CACHE_PATH" ]; then
  scons_args+=("cache_path=$SCONS_CACHE_PATH")
  echo "[gdterm] Using SCons cache: $SCONS_CACHE_PATH"
fi

$PYTHON_BIN -m SCons "${scons_args[@]}" target=template_debug
$PYTHON_BIN -m SCons "${scons_args[@]}" target=template_release
popd >/dev/null

DST="$ENGINE_ROOT/bin/addons/gdterm"
rm -rf "$DST"
mkdir -p "$DST"
cp -R "$BUILD_ROOT/addons/gdterm/." "$DST"

echo "[gdterm] Build finished"

if [ "$KEEP_BUILD" != "1" ] && [ "$reuse_effective" != "1" ]; then
  rm -rf "$BUILD_ROOT"
fi
