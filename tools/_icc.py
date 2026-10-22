"""Minimal ICC v2 RGB display-profile builder.

Only exists to tag a test fixture as non-sRGB; no general ICC support is
needed anywhere else in the repo (lcms2 handles real profiles from Phase 1
onward). Hand-rolled because no library available here exposes "build an
RGB matrix profile from primaries" — see PLAN.md P0.3.
"""
from __future__ import annotations

import struct

_D50 = (0.9642, 1.0000, 0.8249)

# Adobe RGB (1998) colorant tristimulus values, D50-adapted (PCS-relative),
# matching the published reference profile's XYZ tags.
_ADOBE_RGB_PRIMARIES_D50 = {
    "r": (0.60974, 0.31111, 0.01947),
    "g": (0.20528, 0.62567, 0.06087),
    "b": (0.14919, 0.06322, 0.74457),
}
_ADOBE_RGB_GAMMA = 2.19921875


def _s15fixed16(x: float) -> int:
    return int(round(x * 65536))


def _pad4(data: bytes) -> bytes:
    pad = (-len(data)) % 4
    return data + b"\x00" * pad


def _tag_xyz(x: float, y: float, z: float) -> bytes:
    return struct.pack(">4sI iii", b"XYZ ", 0, _s15fixed16(x), _s15fixed16(y), _s15fixed16(z))


def _tag_curv_gamma(gamma: float) -> bytes:
    return struct.pack(">4sIIH", b"curv", 0, 1, int(round(gamma * 256)))


def _tag_desc(text: str) -> bytes:
    ascii_bytes = text.encode("ascii") + b"\x00"
    head = struct.pack(">4sII", b"desc", 0, len(ascii_bytes))
    tail = struct.pack(">IIHB", 0, 0, 0, 0) + b"\x00" * 67
    return head + ascii_bytes + tail


def _tag_text(text: str) -> bytes:
    return struct.pack(">4sI", b"text", 0) + text.encode("ascii") + b"\x00"


def build_adobe_rgb_icc_profile() -> bytes:
    """Return bytes of a minimal, valid ICC v2.4 profile approximating Adobe RGB (1998)."""
    tags = {
        "desc": _tag_desc("Synthetic Adobe RGB-like (test fixture)"),
        "cprt": _tag_text("Public domain colorimetric constants; generated for LumaTune tests"),
        "wtpt": _tag_xyz(*_D50),
        "rXYZ": _tag_xyz(*_ADOBE_RGB_PRIMARIES_D50["r"]),
        "gXYZ": _tag_xyz(*_ADOBE_RGB_PRIMARIES_D50["g"]),
        "bXYZ": _tag_xyz(*_ADOBE_RGB_PRIMARIES_D50["b"]),
        "rTRC": _tag_curv_gamma(_ADOBE_RGB_GAMMA),
        "gTRC": _tag_curv_gamma(_ADOBE_RGB_GAMMA),
        "bTRC": _tag_curv_gamma(_ADOBE_RGB_GAMMA),
    }

    header_size = 128
    tag_table_size = 4 + 12 * len(tags)
    offset = header_size + tag_table_size

    tag_table_entries = []
    tag_data = b""
    for sig, data in tags.items():
        padded = _pad4(data)
        tag_table_entries.append(struct.pack(">4sII", sig.encode("ascii"), offset, len(data)))
        tag_data += padded
        offset += len(padded)

    total_size = offset

    header = bytearray(128)
    struct.pack_into(">I", header, 0, total_size)
    struct.pack_into(">4s", header, 4, b"\x00\x00\x00\x00")  # CMM type: none
    struct.pack_into(">I", header, 8, 0x02400000)  # version 2.4.0
    struct.pack_into(">4s", header, 12, b"mntr")  # device class: display
    struct.pack_into(">4s", header, 16, b"RGB ")  # data colour space
    struct.pack_into(">4s", header, 20, b"XYZ ")  # PCS
    struct.pack_into(">4s", header, 36, b"acsp")  # file signature
    struct.pack_into(">I", header, 64, 0)  # rendering intent: perceptual
    struct.pack_into(">iii", header, 68, *(_s15fixed16(v) for v in _D50))  # PCS illuminant

    body = struct.pack(">I", len(tags)) + b"".join(tag_table_entries) + tag_data
    return bytes(header) + body
