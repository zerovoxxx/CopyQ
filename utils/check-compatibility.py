#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check SPEC2's migration inventory against the public source entrances."""
from pathlib import Path
import re
import sys


def inventory(root):
    actions = (root / "src/gui/menuitems.h").read_text(encoding="utf-8").split("enum Id {")[1].split("Count")[0]
    result = {"action": set(re.findall(r"^\s+(\w+),", actions, re.M))}
    result["option"] = set(re.findall(r"^struct (\w+) : Config", (root / "src/common/appconfig.h").read_text(encoding="utf-8"), re.M))
    result["form"] = {p.relative_to(root).as_posix() for p in (root / "src/ui").glob("*.ui")}
    result["plugin-form"] = {p.relative_to(root).as_posix() for p in (root / "plugins").glob("*/*.ui")}
    result["plugin"] = {p.parent.name for p in (root / "plugins").glob("*/CMakeLists.txt")}
    api = (root / "docs/scripting-api.rst").read_text(encoding="utf-8")
    result["api"] = set()
    for block in re.findall(r"^\.\. js:function::[^\n]+(?:\n {17}\S[^\n]*)*", api, re.M):
        for line in block.splitlines():
            signature = line.removeprefix(".. js:function::").strip()
            match = re.match(r"(?:/\*.*?\*/\s*)?(\w+)\(", signature)
            if match:
                result["api"].add(match.group(1))
    return result


def main():
    root = Path(__file__).resolve().parent.parent
    spec = (root / "docs/astack/version/Iteration2_CopyQCompatibility_SPEC.md").read_text(encoding="utf-8")
    errors = []
    for kind, expected in inventory(root).items():
        documented = set(re.findall(r"`" + re.escape(kind) + r":([^`]+)`", spec))
        for name in sorted(expected - documented):
            errors.append(f"Missing {kind}: {name}")
        for name in sorted(documented - expected):
            errors.append(f"Stale {kind}: {name}")
        print(f"{kind}: {len(expected)} source entrances, {len(documented)} documented")
    print("\n".join(errors) if errors else "Compatibility inventory: PASS (migration status is recorded in SPEC2)")
    return bool(errors)


if __name__ == "__main__":
    sys.exit(main())
