"""Generate decoder/IO unit-test fixtures (PLAN.md P1.2, test T-2).

Separate from tools/gen_fixtures.py (P0.3's golden-pipeline fixtures,
tests/golden/fixtures/) - these are for StdDecoder unit tests only
(tests/unit/fixtures/), procedurally generated and deterministic for the
same reason: no photo to license-check, byte-identical on every re-run.

Usage: python tools/gen_io_test_fixtures.py
"""
from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np
from PIL import Image

OUT_DIR = Path(__file__).resolve().parent.parent / "tests" / "unit" / "fixtures"
QUAD = 32  # each quadrant is QUAD x QUAD; image is 2*QUAD x 2*QUAD

# Canonical *displayed* (i.e. post-orientation-correction) image: four
# solid-colour quadrants. TL=red, TR=green, BL=blue, BR=white.
_RED, _GREEN, _BLUE, _WHITE = (255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 255)


def _canonical() -> Image.Image:
    img = Image.new("RGB", (2 * QUAD, 2 * QUAD))
    img.paste(_RED, (0, 0, QUAD, QUAD))
    img.paste(_GREEN, (QUAD, 0, 2 * QUAD, QUAD))
    img.paste(_BLUE, (0, QUAD, QUAD, 2 * QUAD))
    img.paste(_WHITE, (QUAD, QUAD, 2 * QUAD, 2 * QUAD))
    return img


# Inverse of Pillow's own ImageOps.exif_transpose correction table (verified
# empirically against Pillow's source - see PR description): applying this
# transpose to `_canonical()` produces the pixels a camera would *store* for
# a file carrying that orientation tag, so that correcting it reproduces
# the canonical image exactly.
_INVERSE_TRANSPOSE = {
    1: None,
    2: Image.FLIP_LEFT_RIGHT,
    3: Image.ROTATE_180,
    4: Image.FLIP_TOP_BOTTOM,
    5: Image.TRANSPOSE,
    6: Image.ROTATE_90,
    7: Image.TRANSVERSE,
    8: Image.ROTATE_270,
}


def gen_exif_orientation_set() -> None:
    canonical = _canonical()
    for tag, method in _INVERSE_TRANSPOSE.items():
        stored = canonical if method is None else canonical.transpose(method)
        exif = stored.getexif()
        exif[0x0112] = tag
        stored.save(OUT_DIR / f"exif_orientation_{tag}.jpg", exif=exif, quality=95)


def gen_sixteen_bit_tiff() -> None:
    img8 = np.array(_canonical())
    img16 = img8.astype(np.uint16) * 257  # 0-255 -> 0-65535
    bgr16 = img16[:, :, ::-1]
    cv2.imwrite(str(OUT_DIR / "sixteen_bit.tiff"), bgr16)


def gen_sixteen_bit_precision_png() -> None:
    # Two solid halves at 16-bit values that round to the *same* 8-bit
    # level (10000 and 10100 both round to 39/255) but are ~0.0015 apart
    # as true 16-bit floats. A decoder that silently downcasts to 8-bit
    # (PLAN.md A-3) would show these as identical; a correct one won't.
    h, w = 16, 32
    img16 = np.empty((h, w, 3), dtype=np.uint16)
    img16[:, : w // 2] = 10000
    img16[:, w // 2 :] = 10100
    cv2.imwrite(str(OUT_DIR / "sixteen_bit_precision.png"), img16[:, :, ::-1])


def gen_corrupt_png() -> None:
    # Valid PNG signature, garbage after it - a decoder must fail cleanly,
    # not crash, on this.
    (OUT_DIR / "corrupt.png").write_bytes(b"\x89PNG\r\n\x1a\n" + b"not actually png data" * 4)


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    gen_exif_orientation_set()
    gen_sixteen_bit_tiff()
    gen_sixteen_bit_precision_png()
    gen_corrupt_png()
    print(f"Wrote {len(list(OUT_DIR.iterdir()))} files to {OUT_DIR}")


if __name__ == "__main__":
    main()
