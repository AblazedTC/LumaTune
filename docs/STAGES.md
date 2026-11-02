# Stages

One row per stage that exists in `src/core/`. AGENTS.md §9: a PR that adds a stage must add a row
here, a unit test, a benchmark, and typed diagnostics (where there's anything meaningful to
diagnose — see the note on `ToLinear`/`Encode` below).

| Stage | File | Transfer in → out | Bypassable | Diagnostics | Notes |
|---|---|---|---|---|---|
| `toLinearStage` | `stages/classical/ToLinear.cpp` | sRGB/Gamma22/ICC → Linear | No (fixed domain boundary) | None — see below | Dispatches on `ColorSpace`; fails with `ColorError::UnsupportedPrimaries` for non-sRGB primaries with no ICC profile to convert them, and `ColorError::IccParseFailed`/`IccTransformFailed` for a bad ICC profile |
| `encodeStage` | `stages/classical/Encode.cpp` | Linear → sRGB | No (fixed domain boundary) | None — see below | The only place the sRGB OETF is applied (PLAN.md §7.2); clamps to `[0,1]`; fails with `ColorError::WrongTransfer` if the input isn't already Linear |
| `whiteBalance` | `stages/classical/WhiteBalance.cpp` | Linear → Linear | Yes (`WhiteBalanceParams.bypass`) | `WbDiag` | Four estimators behind one formula shape (PLAN.md §7.3): GrayWorld (mean), WhitePatch (99th percentile), ShadesOfGray (Minkowski p-norm mean), GrayEdge (p-norm mean of gradient magnitude). Fails with `ColorError::UnsupportedFormat` for anything other than `RGB_F32` |
| `autoLevels` | `stages/classical/AutoLevels.cpp` | Lab (no `Transfer` tag; see below) | Yes (`LevelsParams.bypass`) | `LevelsDiag` | Stretches L's `[blackPct, whitePct]` percentile range to `[0,100]`, clamped; a/b pass through. Percentiles read off a 4096-bin histogram (PLAN.md §7.4), legacy's zero-width-range guard ported. Operates on `cv::Mat`, not `ImageBuffer` — `CV_Assert`'d shape, not `Expected` (see note below) |
| `curve` | `stages/classical/Curve.cpp` | Lab (no `Transfer` tag) | Yes (`CurveParams.bypass`, defaults `true`) | None — see below | 16-knot piecewise-linear L curve → midpoint-pivoted contrast → gamma, each clamped to `[0,100]`. New in the rewrite; no V1 equivalent (confirmed against `legacy/processing/tone_curve.py`, which is just a `cv2.createCLAHE` wrapper) |
| `saturation` | `stages/classical/Saturation.cpp` | Lab (no `Transfer` tag) | Yes (`SaturationParams.bypass`, defaults `true`) | None — see below | Scales a/b about neutral (0) by `scale`; L passes through. New in the rewrite; no V1 equivalent |

**Why `ToLinear`/`Encode`/`curve`/`saturation` have no typed diagnostic struct:** PLAN.md §6.3's
`Diagnostics` aggregate lists `WbDiag`, `LevelsDiag`, `ClaheDiag`, histograms, and timings — no
`ToLinearDiag`/`EncodeDiag`/`CurveDiag`/`SaturationDiag`. `ToLinear`/`Encode` have no user-facing
parameters at all; `curve`/`saturation` have parameters but no *estimated* value to report back
(unlike `autoLevels`'s percentile-derived black/white points, every field of `CurveParams`/
`SaturationParams` is already known before the stage runs, so there's nothing a diagnostics
struct would add over the `Params` the caller passed in). `StageTiming` (PLAN.md §6.3, lands with
the engine in P1.6) will still record their timing like any other stage. Revisit this row if a
future version of either stage adds an estimation step.

**Lab stages have no `Transfer` tag:** `autoLevels`/`curve`/`saturation` operate on a plain
`cv::Mat` (CV_32FC3, L∈[0,100]), not `ImageBuffer` — Lab is transient and never stored in an
`ImageBuffer` (PLAN.md §6.1). There's therefore no caller-controlled runtime tag to validate, so
each is `CV_Assert`'d on shape/depth rather than returning `Expected` — documented in each
header as a programmer error, not a recoverable one (same reasoning as `WhiteBalance`'s
`Expected`-returning check, inverted: there, `PixelFormat` *is* caller-controlled).

**Golden-set entries:** `ToLinear`/`Encode`/`WhiteBalance`/`autoLevels`/`curve`/`saturation` don't
have a `tests/golden/` entry of their own yet. T-3 (golden images, PLAN.md §11) is a
*full-pipeline* legacy-parity comparison — there's no legacy equivalent for an isolated
intermediate stage's output (and `curve` has no legacy equivalent at all), and the first point
where a real pipeline chain (and therefore a meaningful golden comparison) exists is P1.6. Once
P1.6 lands, this row should be revisited.

**Benchmarks:** see `tests/bench/StageBench.cpp`.
