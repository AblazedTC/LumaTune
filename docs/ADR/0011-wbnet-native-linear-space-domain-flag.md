# ADR-0011: WbNet operates in the input's native linear space with a domain flag (DR-11)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §3 DR-11, §17.2.1

## Decision

WbNet (V4's learned white-balance model) consumes camera RGB for RAW and linear sRGB for
JPEG/PNG — never a single normalised space — with a `{camera, srgb}` one-hot `domain` input
telling it which. Training data (Cube++) is prepared in **both** domains: camera-space linear
PNGs as shipped, and a derived linear-sRGB version via each image's camera→XYZ→sRGB matrix
applied to both pixels and the ground-truth illuminant.

The network predicts the **illuminant** (log-chroma `u=log(R/G), v=log(B/G)`), never gains;
correction gains are derived outside the model as `g = 1/ℓ`, normalised so `g_G = 1`.

## Consequences

- WbNet cannot be fed an arbitrary RGB buffer without knowing which domain it came from — the
  domain flag is load-bearing, not cosmetic.
- Angular error (the primary eval metric) is always computed between predicted and
  ground-truth **illuminants**, never between gain vectors — keeps the metric domain-invariant.
- Explainability requirement (PLAN.md §17.1) holds: the model's output is a physically meaningful
  parameter (illuminant), editable and inspectable in Diagnostics, never an opaque image result.
