"""Run the legacy V1 pipeline on every fixture x 4 param sets; write expected
PNGs + hashes.json (PLAN.md P0.2). The legacy app is the reference oracle
until Phase 3 (P3.8) swaps these for byte-exact C++ goldens.

Usage: python tools/gen_golden.py
"""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

import cv2
import numpy as np

REPO_ROOT = Path(__file__).resolve().parent.parent
LEGACY_DIR = REPO_ROOT / "legacy"
FIXTURES_DIR = REPO_ROOT / "tests" / "golden" / "fixtures"
EXPECTED_DIR = REPO_ROOT / "tests" / "golden" / "expected"
HASHES_PATH = REPO_ROOT / "tests" / "golden" / "hashes.json"
FIXTURE_EXTS = {".png", ".jpg", ".jpeg", ".webp"}

sys.path.insert(0, str(LEGACY_DIR))

from models.image_state import ImageState  # noqa: E402
from processing.pipeline import Pipeline, ProcessingParams  # noqa: E402

PARAM_SETS = {
    "default": ProcessingParams(),
    "subtle": ProcessingParams(black_pct=1.0, white_pct=99.0, clahe_clip=1.0, clahe_tile=8),
    "punchy": ProcessingParams(black_pct=0.1, white_pct=99.9, clahe_clip=4.0, clahe_tile=8),
    "fine_tile": ProcessingParams(black_pct=0.5, white_pct=99.5, clahe_clip=2.0, clahe_tile=16),
}


def main() -> None:
    EXPECTED_DIR.mkdir(parents=True, exist_ok=True)
    fixtures = sorted(p for p in FIXTURES_DIR.iterdir() if p.suffix.lower() in FIXTURE_EXTS)

    entries = []
    for fixture_path in fixtures:
        state = ImageState.load(str(fixture_path))
        for set_name, params in PARAM_SETS.items():
            output_float, _diag = Pipeline.run(state, params)
            output_u8 = np.clip(output_float * 255.0, 0, 255).astype(np.uint8)
            out_name = f"{fixture_path.stem}__{set_name}.png"
            out_path = EXPECTED_DIR / out_name
            bgr = cv2.cvtColor(output_u8, cv2.COLOR_RGB2BGR)
            cv2.imwrite(str(out_path), bgr)
            entries.append(
                {
                    "fixture": fixture_path.name,
                    "param_set": set_name,
                    "params": vars(params),
                    "output": out_name,
                    "sha256": hashlib.sha256(out_path.read_bytes()).hexdigest(),
                }
            )

    HASHES_PATH.write_text(json.dumps({"entries": entries}, indent=2) + "\n")
    print(f"Wrote {len(entries)} expected images -> {EXPECTED_DIR}")
    print(f"Wrote {HASHES_PATH}")


if __name__ == "__main__":
    main()
