# ADR-0009: Own float CLAHE implementation, not cv::CLAHE (DR-9)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §3 DR-9, §7.4

## Decision

OpenCV's `cv::CLAHE` only accepts `CV_8UC1`/`CV_16UC1`; its OpenCL path is 8-bit only, and its
16-bit CPU path uses a 65,536-bin histogram, so `clipLimit` means something different at every
tile size and image resolution. None of that matches our needs (float L, GPU parity, V1-comparable
clip semantics).

`luma::Clahe` is implemented ourselves: float input, configurable `bins` (default 1024; 256
reproduces V1's 8-bit semantics for parity tests), clip limit defined as `clip × tilePixels / bins`
— exactly OpenCV's 8-bit formula — bilinear tile interpolation, `cv::parallel_for_` over tiles.

## Consequences

- `cv::createCLAHE` must never appear in production code (AGENTS.md §14) — `luma::Clahe` is the
  only path.
- GPU acceleration of CLAHE is explicitly deferred (risk R-10); it is a known chain-break point
  (ADR-0006) until profiling says otherwise.
- Golden-test tolerances for CLAHE-containing pipelines must account for `bins` choice: 256 for
  V1 parity mode, 1024 for the shipping default.
