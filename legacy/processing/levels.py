from __future__ import annotations

import numpy as np


def compute_auto_levels(
    L: np.ndarray,
    black_pct: float = 0.5,
    white_pct: float = 99.5,
) -> tuple[float, float, dict]:
    """Return (black_point, white_point) from percentile analysis of the L channel."""
    black_pt = float(np.percentile(L, black_pct))
    white_pt = float(np.percentile(L, white_pct))

    # Guard against a zero-width range
    if white_pt <= black_pt:
        white_pt = black_pt + 1.0

    diag = {
        "black_point": black_pt,
        "white_point": white_pt,
        "black_pct": black_pct,
        "white_pct": white_pct,
        "original_min": float(L.min()),
        "original_max": float(L.max()),
    }
    return black_pt, white_pt, diag


def apply_levels(L: np.ndarray, black_pt: float, white_pt: float) -> np.ndarray:
    """Stretch [black_pt, white_pt] to [0, 255], clamping values outside that range."""
    stretched = (L.astype(np.float32) - black_pt) / (white_pt - black_pt) * 255.0
    return np.clip(stretched, 0, 255).astype(np.uint8)
