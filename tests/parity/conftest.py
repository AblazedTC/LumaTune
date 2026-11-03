"""T-5 (PLAN.md §11): luma_py vs legacy V1, same arrays. Needs `pip
install -e bindings/python` done first (AGENTS.md) so `import luma_py`
resolves; legacy/ is reached via sys.path, same pattern as
tools/gen_golden.py.
"""
from __future__ import annotations

import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
LEGACY_DIR = REPO_ROOT / "legacy"
FIXTURES_DIR = REPO_ROOT / "tests" / "golden" / "fixtures"

sys.path.insert(0, str(LEGACY_DIR))
