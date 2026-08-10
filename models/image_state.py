from __future__ import annotations

from dataclasses import dataclass, field

import cv2
import numpy as np


@dataclass
class ImageState:
    original_rgb: np.ndarray | None = field(default=None)
    source_path: str = ""

    @classmethod
    def load(cls, path: str) -> "ImageState":
        bgr = cv2.imread(path)
        if bgr is None:
            raise ValueError(f"Could not read image: {path}")
        rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
        rgb_float = rgb.astype(np.float32) / 255.0
        return cls(original_rgb=rgb_float, source_path=path)
