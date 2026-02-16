#!/usr/bin/env bash
set -euo pipefail

PLATFORM="${1:-linux}"
ENGINE_ROOT="${2:-"$(cd "$(dirname "$0")/../.." && pwd)"}"
NODE_VERSION="${3:-20.19.0}"

DEST_ROOT="${ENGINE_ROOT}/bin/tools/node"
TMP_ROOT="$(mktemp -d)"

cleanup() {
	rm -rf "$TMP_ROOT"
}
trap cleanup EXIT

require_cmd() {
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "[node-runtime] Missing required command: $1" >&2
		exit 1
	fi
}

require_cmd curl
require_cmd tar

download_and_stage_unix() {
	local archive_name="$1"
	local extracted_folder="$2"
	local dest_subdir="$3"
	local unpack_flag="$4"

	local archive_path="${TMP_ROOT}/${archive_name}"
	local url="https://nodejs.org/dist/v${NODE_VERSION}/${archive_name}"

	echo "[node-runtime] Downloading ${url}"
	curl -fsSL "$url" -o "$archive_path"

	mkdir -p "${TMP_ROOT}/extract"
	tar "${unpack_flag}" "$archive_path" -C "${TMP_ROOT}/extract"

	local src_node="${TMP_ROOT}/extract/${extracted_folder}/bin/node"
	if [ ! -f "$src_node" ]; then
		echo "[node-runtime] Expected node binary not found at ${src_node}" >&2
		exit 1
	fi

	local dst_dir="${DEST_ROOT}/${dest_subdir}"
	mkdir -p "$dst_dir"
	cp "$src_node" "${dst_dir}/node"
	chmod +x "${dst_dir}/node"
}

mkdir -p "$DEST_ROOT"

case "$PLATFORM" in
	linux)
		rm -rf "${DEST_ROOT}/linux"
		download_and_stage_unix "node-v${NODE_VERSION}-linux-x64.tar.xz" "node-v${NODE_VERSION}-linux-x64" "linux" "-xJf"
		if [ ! -x "${DEST_ROOT}/linux/node" ]; then
			echo "[node-runtime] Failed to stage Linux node runtime." >&2
			exit 1
		fi
		;;
	macos)
		rm -rf "${DEST_ROOT}/macos" "${DEST_ROOT}/macos-arm64" "${DEST_ROOT}/macos-x64"
		download_and_stage_unix "node-v${NODE_VERSION}-darwin-arm64.tar.gz" "node-v${NODE_VERSION}-darwin-arm64" "macos-arm64" "-xzf"
		download_and_stage_unix "node-v${NODE_VERSION}-darwin-x64.tar.gz" "node-v${NODE_VERSION}-darwin-x64" "macos-x64" "-xzf"
		mkdir -p "${DEST_ROOT}/macos"
		cp "${DEST_ROOT}/macos-arm64/node" "${DEST_ROOT}/macos/node"
		chmod +x "${DEST_ROOT}/macos/node"
		for node_bin in "${DEST_ROOT}/macos/node" "${DEST_ROOT}/macos-arm64/node" "${DEST_ROOT}/macos-x64/node"; do
			if [ ! -x "$node_bin" ]; then
				echo "[node-runtime] Failed to stage macOS node runtime file: ${node_bin}" >&2
				exit 1
			fi
		done
		;;
	*)
		echo "[node-runtime] Unsupported platform: ${PLATFORM}" >&2
		exit 1
		;;
esac

echo "[node-runtime] Staged Node v${NODE_VERSION} runtime for ${PLATFORM}"
