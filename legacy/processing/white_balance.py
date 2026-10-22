from __future__ import annotations

import numpy as np


def gray_world_wb(rgb: np.ndarray) -> tuple[np.ndarray, dict]:
    """Apply Gray World assumption to a float32 RGB image in [0, 1]."""
    avg_r = float(rgb[:, :, 0].mean())
    avg_g = float(rgb[:, :, 1].mean())
    avg_b = float(rgb[:, :, 2].mean())
    gray = (avg_r + avg_g + avg_b) / 3.0

    mult_r = gray / avg_r if avg_r > 0 else 1.0
    mult_g = gray / avg_g if avg_g > 0 else 1.0
    mult_b = gray / avg_b if avg_b > 0 else 1.0

    corrected = rgb.copy()
    corrected[:, :, 0] = np.clip(rgb[:, :, 0] * mult_r, 0.0, 1.0)
    corrected[:, :, 1] = np.clip(rgb[:, :, 1] * mult_g, 0.0, 1.0)
    corrected[:, :, 2] = np.clip(rgb[:, :, 2] * mult_b, 0.0, 1.0)

    diag = {
        # Store as 0-255 scale for the diagnostics display
        "avg_r": avg_r * 255.0,
        "avg_g": avg_g * 255.0,
        "avg_b": avg_b * 255.0,
        "mult_r": mult_r,
        "mult_g": mult_g,
        "mult_b": mult_b,
    }
    return corrected, diag
