# ADR-0006: OpenCV T-API with explicit opt-in, device-resident chain, automatic fallback (DR-6)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §9 DR-6

## Decision

A stage runs on `UMat` only if `Settings.gpu == On` **and** `cv::ocl::useOpenCL()` **and** the
stage itself supports it (`supportsUMat()`). `cvtColor`, `resize`, arithmetic, and the Levels
stretch dispatch to OpenCL via `InputArray`/`OutputArray`. CLAHE is our own CPU implementation
(ADR-0009) and currently breaks the device chain: the engine downloads L before CLAHE and the
remaining Lab stages run on the host. Every forced download is recorded as
`StageTiming.gpuChainBreak=true`.

A startup probe (1 MP `cvtColor`+`resize`, including upload and download) decides `gpu=On/Off`
for the session; if OpenCL is slower or throws, it's `Off`.

Custom kernels (Vulkan/Metal/raw OpenCL) are explicitly **not** a goal — only revisited (risk
R-10) if profiling shows the host-side CLAHE chain break dominates. V4 inference GPU use (ORT
execution providers) is a separate concern (ADR-0003, PLAN.md §17.7).

## Consequences

- GPU acceleration is measured per-machine at startup, never assumed from hardware class.
- P1.10's acceptance gate requires an end-to-end (incl. transfer) benchmark showing ≥ 1.3× on the
  reference machine's integrated GPU, or GPU defaults to Off even if the probe passes.
- `ImageBuffer` must support device-resident storage (`DeviceStorage`) so chained UMat-capable
  stages don't round-trip through host memory between each other.
