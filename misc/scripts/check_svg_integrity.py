#!/usr/bin/env python3

import os
import re
import sys
from typing import Iterable, List

DATA_URI_IMAGE_PATTERN = re.compile(
    br"<image\b[^>]*\b(?:href|xlink:href)\s*=\s*['\"]data:image",
    re.IGNORECASE,
)


def is_test_fixture_path(path: str) -> bool:
    normalized = path.replace("\\", "/").lower()
    return (
        normalized.startswith("tests/data/")
        or normalized.startswith("test/data/")
        or "/tests/data/" in normalized
        or "/test/data/" in normalized
        or normalized.startswith("fixtures/")
        or "/fixtures/" in normalized
    )


def check_svg(path: str, errors: List[str]) -> None:
    if is_test_fixture_path(path):
        return

    if os.path.getsize(path) == 0:
        errors.append(f"zero-byte SVG: {path}")
        return

    with open(path, "rb") as file:
        data = file.read()

    if DATA_URI_IMAGE_PATTERN.search(data):
        errors.append(f"embedded data:image SVG <image>: {path}")


def main(paths: Iterable[str]) -> int:
    errors: List[str] = []

    for path in paths:
        if not path.lower().endswith(".svg"):
            continue
        if not os.path.isfile(path):
            continue
        check_svg(path, errors)

    if not errors:
        return 0

    print("SVG integrity check failed:")
    for error in errors:
        print(f"  - {error}")
    print("Use vector-only SVG content and keep SVG files non-empty.")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
