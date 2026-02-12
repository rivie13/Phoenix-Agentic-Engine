import argparse
import json
import os
from pathlib import Path


def _sanitize_extension_api(data: object) -> int:
	"""Ensure all method argument entries have a non-empty `name`.

	Some downstream generators assume arguments always have a valid identifier.
	When the dump contains missing/empty names, generated C++ may contain `, )`.
	"""

	replaced = 0

	def sanitize_args(arg_list: object) -> None:
		nonlocal replaced
		if not isinstance(arg_list, list):
			return
		for index, arg in enumerate(arg_list):
			if not isinstance(arg, dict):
				continue
			name = arg.get("name")
			if name is None or (isinstance(name, str) and name.strip() == ""):
				arg["name"] = f"arg{index}"
				replaced += 1

	def walk(node: object) -> None:
		if isinstance(node, dict):
			for key, value in node.items():
				if key in ("arguments", "args"):
					sanitize_args(value)
				walk(value)
			return
		if isinstance(node, list):
			for item in node:
				walk(item)

	walk(data)
	return replaced


def main() -> int:
	parser = argparse.ArgumentParser(description="Sanitize Godot extension_api.json in-place.")
	parser.add_argument("api_file", help="Path to extension_api.json")
	args = parser.parse_args()

	api_path = Path(args.api_file)
	if not api_path.exists():
		raise SystemExit(f"File not found: {api_path}")

	data = json.loads(api_path.read_text(encoding="utf-8"))
	replaced = _sanitize_extension_api(data)
	if replaced == 0:
		print(f"[sanitize_extension_api] No changes needed: {api_path}")
		return 0

	api_path.write_text(
		json.dumps(data, indent="\t", ensure_ascii=False) + "\n",
		encoding="utf-8",
	)
	print(f"[sanitize_extension_api] Filled {replaced} unnamed arguments: {api_path}")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
