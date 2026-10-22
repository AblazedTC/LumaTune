"""Fail if src/core/** includes any Qt header (AGENTS.md §5 rule 1).

Usage: python tools/check_includes.py
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

CORE_DIR = Path(__file__).resolve().parent.parent / "src" / "core"
QT_INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]Q[A-Za-z]')


def main() -> int:
    violations = []
    for path in CORE_DIR.rglob("*"):
        if path.suffix not in {".h", ".hpp", ".cpp", ".cc"}:
            continue
        for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            if QT_INCLUDE_RE.match(line):
                violations.append(f"{path}:{lineno}: {line.strip()}")

    if violations:
        print("luma_core must not include Qt headers:", file=sys.stderr)
        for v in violations:
            print(f"  {v}", file=sys.stderr)
        return 1

    print(f"OK: no Qt includes under {CORE_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
