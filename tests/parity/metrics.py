"""PSNR + CIEDE2000 for the parity suite. The CIEDE2000 port here is the
same formula (and was cross-checked the same way, against 20000 random
Lab pairs) as tests/support/ImageMetrics.h's C++ version used by the
golden tests (PLAN.md T-3) - kept as a second from-scratch implementation
rather than adding scikit-image as a new test dependency for one metric.
"""
from __future__ import annotations

import math

import cv2
import numpy as np


def psnr(a_u8: np.ndarray, b_u8: np.ndarray) -> float:
    a = a_u8.astype(np.float64)
    b = b_u8.astype(np.float64)
    mse = float(np.mean((a - b) ** 2))
    if mse == 0.0:
        return float("inf")
    return 10.0 * math.log10((255.0**2) / mse)


def _ciede2000(lab1: np.ndarray, lab2: np.ndarray) -> np.ndarray:
    L1, a1, b1 = lab1[..., 0], lab1[..., 1], lab1[..., 2]
    L2, a2, b2 = lab2[..., 0], lab2[..., 1], lab2[..., 2]
    C1 = np.sqrt(a1**2 + b1**2)
    C2 = np.sqrt(a2**2 + b2**2)
    cbar7 = ((C1 + C2) / 2.0) ** 7
    g = 0.5 * (1.0 - np.sqrt(cbar7 / (cbar7 + 25.0**7)))
    a1p = (1.0 + g) * a1
    a2p = (1.0 + g) * a2
    c1p = np.sqrt(a1p**2 + b1**2)
    c2p = np.sqrt(a2p**2 + b2**2)
    h1p = np.degrees(np.arctan2(b1, a1p)) % 360
    h2p = np.degrees(np.arctan2(b2, a2p)) % 360

    dLp = L2 - L1
    dCp = c2p - c1p
    dhp = h2p - h1p
    dhp = np.where(dhp > 180, dhp - 360, dhp)
    dhp = np.where(dhp < -180, dhp + 360, dhp)
    dhp = np.where((c1p * c2p) == 0, 0, dhp)
    dHp = 2 * np.sqrt(c1p * c2p) * np.sin(np.radians(dhp) / 2.0)

    lBarp = (L1 + L2) / 2.0
    cBarp = (c1p + c2p) / 2.0
    hsum = h1p + h2p
    habs = np.abs(h1p - h2p)
    hBarp = np.where(
        (c1p * c2p) == 0,
        hsum,
        np.where(habs <= 180, hsum / 2.0, np.where(hsum < 360, (hsum + 360) / 2.0, (hsum - 360) / 2.0)),
    )

    t = (
        1
        - 0.17 * np.cos(np.radians(hBarp - 30))
        + 0.24 * np.cos(np.radians(2 * hBarp))
        + 0.32 * np.cos(np.radians(3 * hBarp + 6))
        - 0.20 * np.cos(np.radians(4 * hBarp - 63))
    )
    dTheta = 30 * np.exp(-(((hBarp - 275) / 25.0) ** 2))
    cBarp7 = cBarp**7
    rc = 2 * np.sqrt(cBarp7 / (cBarp7 + 25.0**7))
    sl = 1 + (0.015 * (lBarp - 50) ** 2) / np.sqrt(20 + (lBarp - 50) ** 2)
    sc = 1 + 0.045 * cBarp
    sh = 1 + 0.015 * cBarp * t
    rt = -np.sin(np.radians(2 * dTheta)) * rc

    termL = dLp / sl
    termC = dCp / sc
    termH = dHp / sh
    return np.sqrt(termL**2 + termC**2 + termH**2 + rt * termC * termH)


def mean_delta_e2000(rgb_a_u8: np.ndarray, rgb_b_u8: np.ndarray) -> float:
    lab_a = cv2.cvtColor(rgb_a_u8.astype(np.float32) / 255.0, cv2.COLOR_RGB2Lab).astype(np.float64)
    lab_b = cv2.cvtColor(rgb_b_u8.astype(np.float32) / 255.0, cv2.COLOR_RGB2Lab).astype(np.float64)
    return float(np.mean(_ciede2000(lab_a, lab_b)))
