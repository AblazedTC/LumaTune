"""Generate the 12 golden-test fixtures (PLAN.md P0.3).

All fixtures are procedurally generated, seeded, and deterministic — no
real photographs, so there is nothing to license-check. Re-running this
script reproduces byte-identical files.

Usage: python tools/gen_fixtures.py
"""
from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np
from PIL import Image

from _icc import build_adobe_rgb_icc_profile

OUT_DIR = Path(__file__).resolve().parent.parent / "tests" / "golden" / "fixtures"
SIZE = (480, 360)  # (w, h); longest edge 480 <= 512


def make_base_scene(seed: int, top_color, bottom_color) -> np.ndarray:
    """RGB uint8 HxWx3: vertical gradient background + a few random shapes."""
    rng = np.random.default_rng(seed)
    w, h = SIZE
    t = np.linspace(0.0, 1.0, h, dtype=np.float32)[:, None, None]
    top = np.array(top_color, dtype=np.float32)
    bottom = np.array(bottom_color, dtype=np.float32)
    canvas = (top * (1 - t) + bottom * t) * np.ones((h, w, 1), dtype=np.float32)
    img = canvas.astype(np.uint8).copy()

    for _ in range(6):
        color = tuple(int(c) for c in rng.integers(0, 256, size=3))
        center = (int(rng.integers(0, w)), int(rng.integers(0, h)))
        if rng.random() < 0.5:
            radius = int(rng.integers(20, 70))
            cv2.circle(img, center, radius, color, -1)
        else:
            size_ = (int(rng.integers(30, 100)), int(rng.integers(30, 100)))
            pt1 = (max(center[0] - size_[0] // 2, 0), max(center[1] - size_[1] // 2, 0))
            pt2 = (min(center[0] + size_[0] // 2, w - 1), min(center[1] + size_[1] // 2, h - 1))
            cv2.rectangle(img, pt1, pt2, color, -1)
    return img


def save_cv(name: str, rgb_u8: np.ndarray) -> None:
    bgr = cv2.cvtColor(rgb_u8, cv2.COLOR_RGB2BGR)
    cv2.imwrite(str(OUT_DIR / name), bgr)


def save_cv16(name: str, rgb_u16: np.ndarray) -> None:
    bgr = rgb_u16[:, :, ::-1]
    cv2.imwrite(str(OUT_DIR / name), bgr)


def gen_neutral():
    img = make_base_scene(1, (160, 160, 160), (90, 90, 90))
    save_cv("neutral.png", img)


def gen_warm_cast():
    img = make_base_scene(2, (200, 150, 110), (150, 100, 70)).astype(np.float32)
    img[..., 0] *= 1.25  # R up
    img[..., 2] *= 0.75  # B down
    save_cv("warm_cast.jpg", np.clip(img, 0, 255).astype(np.uint8))


def gen_cool_cast():
    img = make_base_scene(3, (110, 150, 200), (70, 100, 150)).astype(np.float32)
    img[..., 0] *= 0.75  # R down
    img[..., 2] *= 1.25  # B up
    save_cv("cool_cast.jpg", np.clip(img, 0, 255).astype(np.uint8))


def gen_dominant_colour():
    # Foliage-like: mostly green, little-to-no neutral/gray content.
    img = make_base_scene(4, (40, 140, 40), (20, 90, 20))
    save_cv("dominant_colour.png", img)


def gen_low_key():
    img = make_base_scene(5, (160, 160, 160), (90, 90, 90)).astype(np.float32)
    img = img * 0.22
    save_cv("low_key.png", np.clip(img, 0, 255).astype(np.uint8))


def gen_high_key():
    img = make_base_scene(6, (160, 160, 160), (90, 90, 90)).astype(np.float32)
    img = 255.0 - (255.0 - img) * 0.22
    save_cv("high_key.png", np.clip(img, 0, 255).astype(np.uint8))


def gen_low_dynamic_range():
    img = make_base_scene(7, (160, 160, 160), (90, 90, 90)).astype(np.float32)
    img = 100.0 + (img / 255.0) * 60.0  # compress into [100, 160]
    save_cv("low_dynamic_range.png", np.clip(img, 0, 255).astype(np.uint8))


def gen_high_dynamic_range():
    img = make_base_scene(8, (200, 200, 200), (20, 20, 20)).astype(np.float32)
    w, h = SIZE
    img[0 : h // 6, :, :] = 0  # clipped-black band
    img[-h // 6 :, :, :] = 255  # clipped-white band
    save_cv("high_dynamic_range.png", np.clip(img, 0, 255).astype(np.uint8))


def gen_sixteen_bit():
    img8 = make_base_scene(9, (160, 160, 160), (90, 90, 90))
    img16 = (img8.astype(np.uint16) * 257)  # 0-255 -> 0-65535
    save_cv16("sixteen_bit.png", img16)


def gen_exif_rotated():
    # Distinct bright rectangle near the top of the *stored* (unrotated)
    # pixel data, so a viewer that honours the EXIF orientation tag ends
    # up showing it on a different edge than one that ignores it.
    img = make_base_scene(10, (160, 160, 160), (90, 90, 90))
    img[0:40, :, :] = (255, 220, 60)
    pil_img = Image.fromarray(img)
    exif = pil_img.getexif()
    exif[0x0112] = 6  # Orientation: rotate 90 CW to display correctly
    pil_img.save(OUT_DIR / "exif_rotated.jpg", exif=exif, quality=90)


def gen_adobergb_tagged():
    img = make_base_scene(11, (160, 160, 160), (90, 90, 90))
    pil_img = Image.fromarray(img)
    pil_img.save(OUT_DIR / "adobergb_tagged.jpg", icc_profile=build_adobe_rgb_icc_profile(), quality=90)


def gen_webp():
    img = make_base_scene(12, (160, 160, 160), (90, 90, 90))
    save_cv("sample.webp", img)


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for fn in (
        gen_neutral,
        gen_warm_cast,
        gen_cool_cast,
        gen_dominant_colour,
        gen_low_key,
        gen_high_key,
        gen_low_dynamic_range,
        gen_high_dynamic_range,
        gen_sixteen_bit,
        gen_exif_rotated,
        gen_adobergb_tagged,
        gen_webp,
    ):
        fn()
    print(f"Wrote {len(list(OUT_DIR.iterdir()))} files to {OUT_DIR}")


if __name__ == "__main__":
    main()
