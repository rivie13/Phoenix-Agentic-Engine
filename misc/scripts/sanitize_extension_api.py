import argparse
import json
from pathlib import Path


def _sanitize_extension_api(data: object) -> int:
    """Sanitize Godot's `extension_api.json` dump for downstream generators.

    Fixes applied:
    - Ensure all argument entries have a non-empty `name`.
      Some downstream generators assume arguments always have a valid identifier.
      When the dump contains missing/empty names, generated C++ may contain `, )`.
    - Remove empty argument lists (e.g. "arguments": []).
      Some downstream generators incorrectly emit a trailing comma when the key exists
      but the list is empty, producing invalid C++ like `_call_native_*(..., );`.
    """

    replaced = 0
    removed_empty_lists = 0

    def sanitize_args(arg_list: object) -> None:
        nonlocal replaced
        if not isinstance(arg_list, list):
            return
        for index, arg in enumerate(arg_list, start=1):
            if not isinstance(arg, dict):
                continue
            name = arg.get("name")
            if name is None or (isinstance(name, str) and name.strip() == ""):
                arg["name"] = f"arg{index}"
                replaced += 1

    def walk(node: object) -> None:
        nonlocal removed_empty_lists
        if isinstance(node, dict):
            for key, value in list(node.items()):
                if key in ("arguments", "args"):
                    if isinstance(value, list) and len(value) == 0:
                        del node[key]
                        removed_empty_lists += 1
                        continue
                    sanitize_args(value)
                walk(value)
            return
        if isinstance(node, list):
            for item in node:
                walk(item)
            return

    walk(data)
    return replaced + removed_empty_lists


def main() -> int:
    parser = argparse.ArgumentParser(description="Sanitize Godot extension_api.json in-place.")
    parser.add_argument("api_file", help="Path to extension_api.json")
    args = parser.parse_args()

    api_path = Path(args.api_file)
    if not api_path.exists():
        raise SystemExit(f"File not found: {api_path}")

    # Use utf-8-sig to tolerate a UTF-8 BOM if present.
    data = json.loads(api_path.read_text(encoding="utf-8-sig"))
    changes = _sanitize_extension_api(data)
    if changes == 0:
        print(f"[sanitize_extension_api] No changes needed: {api_path}")
        return 0

    api_path.write_text(
        json.dumps(data, indent="\t", ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    print(f"[sanitize_extension_api] Applied {changes} change(s): {api_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
