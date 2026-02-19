#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

IMAGE_TAG = "godot-mcp-docs:local"
SUBMODULE_REL_PATH = Path("modules/ultimate_ai/external/godot-mcp-docs")
DOCKERFILE_REL_PATH = SUBMODULE_REL_PATH / "deploy" / "Dockerfile"
MARKER_REL_PATH = Path("modules/ultimate_ai/.phoenix_docs_image_revision")


class CommandError(RuntimeError):
    pass


def _run_command(args: list[str], *, check: bool = True) -> subprocess.CompletedProcess[str]:
    try:
        result = subprocess.run(args, check=False, capture_output=True, text=True)
    except FileNotFoundError as exc:
        raise CommandError(f"Command not found: {args[0]}") from exc

    if check and result.returncode != 0:
        stderr = (result.stderr or "").strip()
        raise CommandError(f"Command failed ({' '.join(args)}): {stderr}")

    return result


def _engine_root() -> Path:
    return Path(__file__).resolve().parents[3]


def _submodule_sha(submodule_dir: Path) -> str:
    result = _run_command(["git", "-C", str(submodule_dir), "rev-parse", "HEAD"])
    return result.stdout.strip()


def _read_marker(marker_path: Path) -> str:
    if not marker_path.exists():
        return ""
    return marker_path.read_text(encoding="utf-8").strip()


def _write_marker(marker_path: Path, sha: str) -> None:
    marker_path.parent.mkdir(parents=True, exist_ok=True)
    marker_path.write_text(f"{sha}\n", encoding="utf-8")


def _docker_available() -> None:
    _run_command(["docker", "--version"])


def _docker_daemon_running() -> None:
    _run_command(["docker", "info"])


def _image_exists(tag: str) -> bool:
    result = _run_command(["docker", "image", "inspect", tag], check=False)
    return result.returncode == 0


def _build_image(dockerfile: Path, context_dir: Path, tag: str) -> None:
    _run_command(
        [
            "docker",
            "build",
            "-f",
            str(dockerfile),
            "-t",
            tag,
            str(context_dir),
        ]
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build/refresh local godot-mcp-docs Docker image using submodule SHA marker."
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Skip Docker checks/build and simulate build logic (CI-safe without Docker).",
    )
    parser.add_argument(
        "--force-rebuild",
        action="store_true",
        help="Force rebuild regardless of image/marker status.",
    )
    args = parser.parse_args()

    root = _engine_root()
    submodule_dir = root / SUBMODULE_REL_PATH
    dockerfile = root / DOCKERFILE_REL_PATH
    marker_path = root / MARKER_REL_PATH

    if not submodule_dir.exists():
        print(f"Submodule path not found: {submodule_dir}", file=sys.stderr)
        return 1
    if not dockerfile.exists():
        print(f"Dockerfile not found: {dockerfile}", file=sys.stderr)
        return 1

    current_sha = _submodule_sha(submodule_dir)
    stored_sha = _read_marker(marker_path)
    sha_changed = stored_sha != current_sha

    print(f"Submodule SHA: {current_sha}")
    if stored_sha:
        print(f"Marker SHA:    {stored_sha}")
    else:
        print("Marker SHA:    <missing>")

    if args.dry_run:
        print("Dry-run enabled: skipping Docker availability/daemon/image checks.")
        image_exists = False
    else:
        _docker_available()
        _docker_daemon_running()
        image_exists = _image_exists(IMAGE_TAG)

    reasons: list[str] = []
    if args.force_rebuild:
        reasons.append("forced")
    if not image_exists:
        reasons.append("image missing")
    if sha_changed:
        reasons.append("submodule SHA changed")

    should_rebuild = len(reasons) > 0

    if should_rebuild:
        print(f"Rebuild required ({', '.join(reasons)}).")
        if args.dry_run:
            print(f"Dry-run: would execute: docker build -f {dockerfile} -t {IMAGE_TAG} {submodule_dir}")
        else:
            _build_image(dockerfile, submodule_dir, IMAGE_TAG)
        _write_marker(marker_path, current_sha)
        print(f"Updated marker: {marker_path}")
    else:
        print("Image is up-to-date; no rebuild required.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
