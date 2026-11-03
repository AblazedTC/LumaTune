"""test_pipeline_parity.py deliberately bypasses luma_py.decode() (it
feeds both sides the exact same array, legacy's own decode, for a true
same-input comparison) - this file is the one place decode() itself gets
exercised.
"""
from __future__ import annotations

import numpy as np

import luma_py

from conftest import FIXTURES_DIR


def test_decode_returns_a_valid_float32_rgb_array():
    path = FIXTURES_DIR / "neutral.png"
    array = np.asarray(luma_py.decode(str(path)))

    assert array.dtype == np.float32
    assert array.ndim == 3
    assert array.shape[2] == 3
    assert np.isfinite(array).all()
    assert array.min() >= 0.0
    assert array.max() <= 1.0
