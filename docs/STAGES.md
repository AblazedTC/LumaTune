# Stages

One row per stage that exists in `src/core/`. AGENTS.md §9: a PR that adds a stage must add a row
here, a unit test, a benchmark, and typed diagnostics (where there's anything meaningful to
diagnose — see the note on `ToLinear`/`Encode` below).

| Stage | File | Transfer in → out | Bypassable | Diagnostics | Notes |
|---|---|---|---|---|---|
| `toLinearStage` | `stages/classical/ToLinear.cpp` | sRGB/Gamma22/ICC → Linear | No (fixed domain boundary) | None — see below | Dispatches on `ColorSpace`; fails with `ColorError::UnsupportedPrimaries` for non-sRGB primaries with no ICC profile to convert them, and `ColorError::IccParseFailed`/`IccTransformFailed` for a bad ICC profile |
| `encodeStage` | `stages/classical/Encode.cpp` | Linear → sRGB | No (fixed domain boundary) | None — see below | The only place the sRGB OETF is applied (PLAN.md §7.2); clamps to `[0,1]`; fails with `ColorError::WrongTransfer` if the input isn't already Linear |
| `whiteBalance` | `stages/classical/WhiteBalance.cpp` | Linear → Linear | Yes (`WhiteBalanceParams.bypass`) | `WbDiag` | Four estimators behind one formula shape (PLAN.md §7.3): GrayWorld (mean), WhitePatch (99th percentile), ShadesOfGray (Minkowski p-norm mean), GrayEdge (p-norm mean of gradient magnitude). Fails with `ColorError::UnsupportedFormat` for anything other than `RGB_F32` |

**Why `ToLinear`/`Encode` have no typed diagnostic struct:** PLAN.md §6.3's `Diagnostics`
aggregate lists `WbDiag`, `LevelsDiag`, `ClaheDiag`, histograms, and timings — no
`ToLinearDiag`/`EncodeDiag`. Unlike WhiteBalance/Levels/CLAHE, these two have no user-facing
parameters and no estimation decision to explain; they're fixed domain-conversion boundaries.
`StageTiming` (PLAN.md §6.3, lands with the engine in P1.6) will still record their timing like
any other stage. Revisit this row if a future stage genuinely needs per-run diagnostic data from
either of them.

**Golden-set entries:** `ToLinear`/`Encode`/`WhiteBalance` don't have a `tests/golden/` entry of
their own yet. T-3 (golden images, PLAN.md §11) is a *full-pipeline* legacy-parity comparison —
there's no legacy equivalent for an isolated intermediate stage's output, and the first point
where a real pipeline chain (and therefore a meaningful golden comparison) exists is P1.6. Once
P1.6 lands, this row should be revisited.

**Benchmarks:** see `tests/bench/StageBench.cpp`.
