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
API_FILE="$(python3 -c 'import os,sys; print(os.path.realpath(sys.argv[1]))' "$API_FILE")"
if [ ! -f "$API_FILE" ]; then
  echo "Missing extension_api.json at $API_FILE" >&2
  exit 1
fi
echo "[git-plugin] Using extension API: $API_FILE"
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

ensure_third_party_repo() {
  local repo_url="$1"
  local dest_path="$2"
  local commit="$3"

  if [ ! -d "$dest_path/.git" ]; then
    rm -rf "$dest_path"
    mkdir -p "$(dirname "$dest_path")"
    git clone --depth 1 "$repo_url" "$dest_path"
  fi

  git -C "$dest_path" fetch --depth 1 origin "$commit"
  git -C "$dest_path" checkout --detach "$commit"
}

BUILD_ROOT="${RUNNER_TEMP:-/tmp}/phoenix-git-plugin-build"
rm -rf "$BUILD_ROOT"

if [ "$PLATFORM" = "linuxbsd" ]; then
  PLATFORM="linux"
fi

git clone --depth 1 https://github.com/rivie13/godot-git-plugin.git "$BUILD_ROOT"

pushd "$BUILD_ROOT" >/dev/null
git submodule update --init --recursive --depth 1
ensure_third_party_repo "https://github.com/openssl/openssl.git" "$BUILD_ROOT/thirdparty/openssl" "26baecb28ce461696966dac9ac889629db0b3b96"
ensure_third_party_repo "https://github.com/libgit2/libgit2.git" "$BUILD_ROOT/thirdparty/git2/libgit2" "b7bad55e4bb0a285b073ba5e02b01d3f522fc95d"
ensure_third_party_repo "https://github.com/libssh2/libssh2.git" "$BUILD_ROOT/thirdparty/ssh2/libssh2" "635caa90787220ac3773c1d5ba11f1236c22eae8"

if [ "$PLATFORM" = "macos" ]; then
  python3 - <<'PY'
from pathlib import Path

path = Path("tools/git2.py")
text = path.read_text(encoding="utf-8")
needle = "    if env[\"platform\"] != \"windows\":\n        config[\"CMAKE_C_FLAGS\"] = \"-fPIC\"\n    else:\n        config[\"OPENSSL_ROOT_DIR\"] = env[\"SSL_BUILD\"]\n"
if "single-bit-bitfield-constant-conversion" not in text and needle in text:
    replacement = (
        "    if env[\"platform\"] != \"windows\":\n"
        "        config[\"CMAKE_C_FLAGS\"] = \"-fPIC\"\n"
        "        if env[\"platform\"] == \"macos\":\n"
        "            config[\"CMAKE_C_FLAGS\"] += \" -Wno-single-bit-bitfield-constant-conversion\"\n"
        "    else:\n"
        "        config[\"OPENSSL_ROOT_DIR\"] = env[\"SSL_BUILD\"]\n"
    )
    path.write_text(text.replace(needle, replacement), encoding="utf-8")
PY

  python3 - <<'PY'
from pathlib import Path

path = Path("thirdparty/git2/libgit2/deps/zlib/zutil.h")
text = path.read_text(encoding="utf-8")
needle = "#        define fdopen(fd,mode) NULL /* No fdopen() */"
if needle in text:
  replacement = "#        if !defined(__APPLE__)\n#          define fdopen(fd,mode) NULL /* No fdopen() */\n#        endif"
  path.write_text(text.replace(needle, replacement), encoding="utf-8")
PY
fi

LOCAL_PLUGIN_SOURCE="$ENGINE_ROOT/modules/ultimate_ai/external/godot-git-plugin/godot-git-plugin/src/git_plugin.cpp"
TEMP_PLUGIN_SOURCE="$BUILD_ROOT/godot-git-plugin/src/git_plugin.cpp"
if [ -f "$LOCAL_PLUGIN_SOURCE" ]; then
  cp -f "$LOCAL_PLUGIN_SOURCE" "$TEMP_PLUGIN_SOURCE"
fi

pushd "$BUILD_ROOT/godot-cpp" >/dev/null
git fetch origin
if git show-ref --verify --quiet "refs/remotes/origin/$GODOT_CPP_BRANCH"; then
  git checkout "$GODOT_CPP_BRANCH"
else
  git checkout master
fi
popd >/dev/null
mkdir -p "$BUILD_ROOT/godot-cpp/gdextension"
cp -f "$API_FILE" "$BUILD_ROOT/extension_api.json"
cp -f "$API_FILE" "$BUILD_ROOT/godot-cpp/gdextension/extension_api.json"
if [ ! -f "$BUILD_ROOT/extension_api.json" ]; then
  echo "Failed to stage extension_api.json at $BUILD_ROOT/extension_api.json" >&2
  exit 1
fi
if [ ! -f "$BUILD_ROOT/godot-cpp/gdextension/extension_api.json" ]; then
  echo "Failed to stage extension_api.json at $BUILD_ROOT/godot-cpp/gdextension/extension_api.json" >&2
  exit 1
fi
python3 -m SCons platform="$PLATFORM" target=editor dev_build=yes generate_bindings=yes
popd >/dev/null

DST="$ENGINE_ROOT/bin/addons/godot-git-plugin"
rm -rf "$DST"
mkdir -p "$DST"
cp -R "$BUILD_ROOT/addons/godot-git-plugin/." "$DST"

if [ "$KEEP_BUILD" != "1" ]; then
  rm -rf "$BUILD_ROOT"
fi
