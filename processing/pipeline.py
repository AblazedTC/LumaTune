from __future__ import annotations

from dataclasses import dataclass

import cv2
import numpy as np

from models.image_state import ImageState
from processing.levels import apply_levels, compute_auto_levels
from processing.tone_curve import apply_clahe
from processing.white_balance import gray_world_wb


@dataclass
class ProcessingParams:
    black_pct: float = 0.5
    white_pct: float = 99.5
    clahe_clip: float = 2.0
    clahe_tile: int = 8


class Pipeline:
    @staticmethod
    def run(state: ImageState, params: ProcessingParams) -> tuple[np.ndarray, dict]:
        """Run the full enhancement pipeline; return (output_rgb_float32, diagnostics)."""
        rgb = state.original_rgb

        # 1. Gray World white balance
        wb_rgb, wb_diag = gray_world_wb(rgb)

        # 2. Convert to Lab for luminance-only processing
        wb_uint8 = (wb_rgb * 255.0).clip(0, 255).astype(np.uint8)
        lab = cv2.cvtColor(wb_uint8, cv2.COLOR_RGB2Lab)

        L = lab[:, :, 0]  # uint8 [0, 255]
        hist_before = _build_hist(L)

        # 3. Percentile auto-levels on L only (preserves chrominance)
        black_pt, white_pt, levels_diag = compute_auto_levels(
            L, params.black_pct, params.white_pct
        )
        L_stretched = apply_levels(L, black_pt, white_pt)

        # 4. CLAHE for local contrast
        L_enhanced = apply_clahe(
            L_stretched,
            params.clahe_clip,
            (params.clahe_tile, params.clahe_tile),
        )
        hist_after = _build_hist(L_enhanced)

        # 5. Reconstruct RGB with original a/b channels
        lab[:, :, 0] = L_enhanced
        output_uint8 = cv2.cvtColor(lab, cv2.COLOR_Lab2RGB)
        output_float = output_uint8.astype(np.float32) / 255.0

        diagnostics = {
            "wb": wb_diag,
            "levels": levels_diag,
            "clahe": {
                "clip_limit": params.clahe_clip,
                "tile_size": params.clahe_tile,
            },
            "hist_before": hist_before,
            "hist_after": hist_after,
        }
        return output_float, diagnostics


def _build_hist(L: np.ndarray) -> np.ndarray:
    hist, _ = np.histogram(L.ravel(), bins=256, range=(0, 255))
    return hist.astype(np.float32)
