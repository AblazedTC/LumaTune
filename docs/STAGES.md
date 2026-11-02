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
| `clahe` | `stages/classical/Clahe.cpp` | Lab (no `Transfer` tag) | Yes (`ClaheParams.bypass`) | `ClaheDiag` | Own float CLAHE (DR-9) — `cv::createCLAHE` is 8/16-bit only with an 8-bit-only OpenCL path, neither fits our float/GPU-parity needs. Per-tile `bins`-bucket histogram, clip limit `clipLimit × tilePixels / bins` (OpenCV's own 8-bit formula) with excess redistributed uniformly, per-tile CDF rescaled to `[0,100]`, bilinear interpolation between the four neighbouring tiles' CDFs, `cv::parallel_for_` over tiles. `bins=256` matches OpenCV's/legacy's 8-bit `cv::CLAHE` within 1 LSB (`ClaheTest.cpp`, direct comparison — not a `tests/golden/` fixture, see below); `bins=1024` is the shipping default. Not yet wired into the OpenCL/`UMat` chain — PLAN.md §9 flags this as a known chain break, deferred |
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

**Lab stages have no `Transfer` tag:** `autoLevels`/`clahe`/`curve`/`saturation` operate on a
plain `cv::Mat` (CV_32FC3, L∈[0,100]), not `ImageBuffer` — Lab is transient and never stored in an
`ImageBuffer` (PLAN.md §6.1). There's therefore no caller-controlled runtime tag to validate, so
each is `CV_Assert`'d on shape/depth rather than returning `Expected` — documented in each
header as a programmer error, not a recoverable one (same reasoning as `WhiteBalance`'s
`Expected`-returning check, inverted: there, `PixelFormat` *is* caller-controlled).

**Golden-set entries:** `ToLinear`/`Encode`/`WhiteBalance`/`autoLevels`/`clahe`/`curve`/
`saturation` don't have a `tests/golden/` entry of their own yet. T-3 (golden images, PLAN.md
§11) is a *full-pipeline* legacy-parity comparison — there's no legacy equivalent for an isolated
intermediate stage's output (and `curve` has no legacy equivalent at all). `src/core/engine/
Pipeline.cpp` (P1.6) now runs a real chain end-to-end, but the T-3 fixtures/legacy-comparison
harness itself (12 images × 4 param sets, PSNR/ΔE00 against checked-in legacy-generated
expected outputs) is still a separate, pending follow-up — see PLAN.md's P1.6 row. `clahe` gets
its DR-9 parity check a different way in the meantime: `ClaheTest.cpp` compares directly against
`cv::createCLAHE` (which legacy's `cv2.createCLAHE` wraps) rather than waiting for a golden
fixture.

**Pipeline orchestration:** `src/core/engine/Pipeline.h` (`runPipeline`) chains ToLinear ->
WhiteBalance -> Encode -> ToLab -> AutoLevels -> Clahe -> Curve -> Saturation -> FromLab
(PLAN.md §7.2) and assembles the `Diagnostics` aggregate (per-stage `StageTiming`, `lumaBefore`/
`lumaAfter`/`rgbAfter` 256-bin histograms). It isn't itself a "stage" (no row above) - it's the
engine's orchestrator, consistent with PLAN.md §7. RAW-only `Exposure` (V2) and the
adaptive/learned stage overrides (V3/V4) aren't wired in since they don't exist yet.

**Benchmarks:** see `tests/bench/StageBench.cpp`.
