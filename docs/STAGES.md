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
`saturation` don't have a `tests/golden/` entry of their own — T-3 (PLAN.md §11) is a
*full-pipeline* comparison, not a per-stage one (there's no legacy equivalent for an isolated
intermediate stage's output, and `curve` has no legacy equivalent at all). `tests/golden/
GoldenTest.cpp` (P1.6) runs the real `runPipeline` chain against all 12 fixtures × 4 param sets
and checks PSNR/mean-ΔE00 against the checked-in legacy-generated expected images for 8 of
them — see PLAN.md's T-3 row for the measured tolerance and why 4 fixtures (the 3 color-cast
ones plus the ICC one) are smoke-checked only. `clahe` additionally gets its own DR-9 parity
check: `ClaheTest.cpp` compares directly against `cv::createCLAHE` (which legacy's
`cv2.createCLAHE` wraps).

**Pipeline orchestration:** `src/core/engine/Pipeline.h` (`runPipeline`) chains ToLinear ->
WhiteBalance -> Encode -> ToLab -> AutoLevels -> Clahe -> Curve -> Saturation -> FromLab
(PLAN.md §7.2) and assembles the `Diagnostics` aggregate (per-stage `StageTiming`, `lumaBefore`/
`lumaAfter`/`rgbAfter` 256-bin histograms). It isn't itself a "stage" (no row above) - it's the
engine's orchestrator, consistent with PLAN.md §7. RAW-only `Exposure` (V2) and the
adaptive/learned stage overrides (V3/V4) aren't wired in since they don't exist yet.

**UMat / GPU path (PLAN.md DR-6/P1.10):** `toLinear`/`toSrgb` (`cv::UMat` overloads) and
`rgbToLab`/`labToRgb` (`cv::InputArray`/`cv::OutputArray` overloads) in `Transforms.h` are
device-transparent - verified to agree with their `cv::Mat` counterparts within 1e-3
(`TransformsTest.cpp`), satisfying P1.10's "same golden results on GPU" half. The other half
("end-to-end bench ≥ 1.3× on reference iGPU, else GPU defaults Off") needs a physical reference
machine this environment doesn't have (PLAN.md §11.4: judged there, never on a CI runner) - not
measured, deliberately deferred. `src/core/engine/GpuProbe.h` (`probeGpuWorthwhile`) implements
the startup-probe *mechanism* DR-6 describes (timed 1 MP `cvtColor`+`resize` on `UMat` vs `Mat`,
including upload/download); it's pure/stateless and doesn't cache or persist a session-wide
`gpu` flag, since there's no `Settings`/session-lifecycle struct yet for it to live in (Phase 2).
Wiring these into `Pipeline.cpp`/`WhiteBalance`/`AutoLevels`/`Clahe` for an actual device-resident
end-to-end chain (not just correctness-verified building blocks) is a separate, still-pending
follow-up - `Clahe` in particular is explicitly a CPU-only chain break by design (DR-9).

**Benchmarks:** see `tests/bench/StageBench.cpp`.
