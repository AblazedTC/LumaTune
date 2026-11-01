# ADR-0007: Use OpenCV's own metadata API; no separate EXIF library (DR-7)

**Status:** Accepted — 2026-11-01
**Plan reference:** PLAN.md §6.4 (IO contracts), task P1.2

## Context

Reading EXIF orientation and an embedded ICC profile needed a library. The candidates on the
table were **exiv2** (GPL-2.0+, rejected outright — forces GPL on the app), **libexif**
(LGPL-2.1, PLAN.md's original default), and **TinyEXIF** (MIT/BSD, JPEG-only).

Prototyping libexif in P1.2 surfaced a problem before any of its EXIF-parsing code was even
reached: its vcpkg port builds via autotools and needs `autoconf`/`autoconf-archive`/`automake`/
`libtool` from the system package manager — not available by default on GitHub's Linux or macOS
hosted runners. That turned out not to be the deciding factor on its own (CI still needs those
autotools packages regardless — `libb2`, pulled in transitively on every platform for a reason
unrelated to EXIF, needs them too — see the CI workflow), but it was still a real cost for a
dependency we didn't strictly need, on top of the LGPL-2.1 obligation.

## Decision

**Neither.** OpenCV 4.13+ (confirmed present in our pinned 4.14.0, against the actual tagged
source and OpenCV's own test suite, not just the header) ships `cv::imreadWithMetadata` /
`cv::imdecodeWithMetadata`, which return the raw EXIF and ICC profile bytes directly as part of
decoding — a dependency we already have for every other reason. Specifically:

- `IMAGE_METADATA_EXIF`: the raw TIFF-structured EXIF blob, with the `"Exif\0\0"` APP1 prefix
  already stripped (starts directly at the `"II"`/`"MM"` byte-order marker) — confirmed against
  `grfmt_jpeg.cpp`'s `EXIF_HEADER_SIZE` offset and `test_exif.cpp`'s `ReadExif_Sanity` check
  (`exif.data[0] == 'I' || exif.data[0] == 'M'`).
- `IMAGE_METADATA_ICCP`: the raw ICC profile payload, APP2 header already stripped.

We only need one EXIF tag (Orientation, 0x0112) read-only — not a full tag dictionary, not
writing — so a ~60-line TIFF-IFD walker (`StdDecoder.cpp`'s `readOrientationTag`) is the
"minimum code that works" rung of the ladder, not a reason to pull in a whole library. `cv::imread`
already auto-rotates pixels to match the tag by default (which is also exactly the behaviour
PLAN.md's `ImageBuffer::exifOrientation` contract wants — "applied at decode"); the tag value is
read only for diagnostics/provenance; P1.3 confirmed export never writes any orientation tag at
all (not even a "corrected" one) — by encode time the pixels are already displayed-correct, so
there is nothing left to tag.

The orientation-value-to-transform table (which of the 8 EXIF tags means which flip/rotate) was
verified against Pillow's own `ImageOps.exif_transpose` source, not derived from memory.

## Consequences

- `libexif` is not a dependency, and its LGPL-2.1 obligation goes away with it. The CI autotools
  step stays regardless (another transitive dependency, `libb2`, needs it too).
- `exiv2`/`TinyEXIF` remain out of scope entirely — not evaluated further, not needed.
- **P1.3 confirmed:** export uses `imwrite`/`imwriteWithMetadata` as expected. ICC bytes are
  re-embedded as raw passthrough (`EncodeParams.preserveIcc`, default on); EXIF is never written
  at all — see the Decision section above.
- If a future task needs *writing* arbitrary EXIF tags (not just orientation) or XMP, this
  decision should be revisited with a new DR, not amended here (PLAN.md §0).
