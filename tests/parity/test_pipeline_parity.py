"""T-5 (PLAN.md §11): luma_py vs legacy V1 "on the same arrays" - decode
once (legacy's own ImageState.load), feed that identical numpy array into
both legacy's Pipeline.run and luma_py.run_pipeline, compare outputs.

Tolerance and fixture categorisation mirror tests/golden/GoldenTest.cpp
exactly (PLAN.md's T-3 row has the measured numbers and the reasoning):
WB's G-normalisation vs legacy's grand-mean normalisation is an accepted
P1.4 design difference, and the ICC fixture is colour-managed by us but
not by legacy, on purpose - full-chain pixel comparison isn't fair for
either, so those 4 fixtures get a smoke check only. The other 8 get a
real PSNR/mean-dE00 check at the same measured-and-padded bound.
"""
from __future__ import annotations

import json

import numpy as np
import pytest

import luma_py
from models.image_state import ImageState
from processing.pipeline import Pipeline, ProcessingParams

from conftest import FIXTURES_DIR
from metrics import mean_delta_e2000, psnr

FIXTURE_EXTS = {".png", ".jpg", ".jpeg", ".webp"}
SMOKE_ONLY = {"adobergb_tagged", "warm_cast", "cool_cast", "dominant_colour"}

PARAM_SETS = {
    "default": ProcessingParams(),
    "subtle": ProcessingParams(black_pct=1.0, white_pct=99.0, clahe_clip=1.0, clahe_tile=8),
    "punchy": ProcessingParams(black_pct=0.1, white_pct=99.9, clahe_clip=4.0, clahe_tile=8),
    "fine_tile": ProcessingParams(black_pct=0.5, white_pct=99.5, clahe_clip=2.0, clahe_tile=16),
}


def _fixtures() -> list:
    return sorted(p for p in FIXTURES_DIR.iterdir() if p.suffix.lower() in FIXTURE_EXTS)


def _to_luma_params_json(params: ProcessingParams) -> str:
    # clahe.bins=256 matches legacy's 8-bit cv2.createCLAHE semantics,
    # same as GoldenTest.cpp (PLAN.md T-3: "the C++ pipeline runs with
    # clahe.bins=256").
    return json.dumps(
        {
            "levels": {"blackPct": params.black_pct, "whitePct": params.white_pct},
            "clahe": {
                "clipLimit": params.clahe_clip,
                "tilesX": params.clahe_tile,
                "tilesY": params.clahe_tile,
                "bins": 256,
            },
        }
    )


@pytest.mark.parametrize("fixture_path", _fixtures(), ids=lambda p: p.stem)
@pytest.mark.parametrize(
    "set_name,params", list(PARAM_SETS.items()), ids=list(PARAM_SETS.keys())
)
def test_pipeline_parity(fixture_path, set_name, params):
    state = ImageState.load(str(fixture_path))
    legacy_out, _legacy_diag = Pipeline.run(state, params)
    legacy_u8 = np.clip(legacy_out * 255.0, 0, 255).astype(np.uint8)

    ours_out, _ours_diag_json = luma_py.run_pipeline(
        state.original_rgb, _to_luma_params_json(params)
    )
    ours_out = np.asarray(ours_out)

    assert ours_out.shape == legacy_out.shape
    assert np.isfinite(ours_out).all()
    assert ours_out.min() >= 0.0
    assert ours_out.max() <= 1.0

    if fixture_path.stem in SMOKE_ONLY:
        return

    ours_u8 = np.clip(ours_out * 255.0, 0, 255).astype(np.uint8)
    p = psnr(ours_u8, legacy_u8)
    mean_de = mean_delta_e2000(ours_u8, legacy_u8)
    assert p >= 22.0, f"{fixture_path.stem}/{set_name}: PSNR={p:.2f}dB"
    assert mean_de <= 9.0, f"{fixture_path.stem}/{set_name}: meanDeltaE00={mean_de:.3f}"
