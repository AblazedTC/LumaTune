# ADR-0004: OpenCV 4.14 with T-API (UMat) for optional OpenCL acceleration (DR-4)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §3 DR-4, §9 (GPU strategy)

## Decision

Colour conversion, resize, and arithmetic are written against `cv::InputArray`/`OutputArray` so
the same code runs on `cv::Mat` (CPU) or `cv::UMat` (OpenCL-resident) without duplication.

⚠️ Whether OpenCL is gated by an explicit `opencl` vcpkg feature or enabled by default must be
confirmed against the pinned baseline in P0.4; the manifest names it explicitly either way. CLAHE
is **not** taken from OpenCV — see ADR-0009.

## Consequences

- GPU use is opt-in and probed at startup (a 1 MP `cvtColor`+`resize` including upload/download);
  if slower than CPU or it throws, `gpu=Off` for the session (ADR-0005/§9 covers the fallback
  policy in full). Expected wins are modest and must be measured, not assumed.
- `ImageBuffer` can hold device-resident storage so UMat-capable stages chain without a host
  round-trip; CLAHE still forces one download per run (documented chain break).
- Custom GPU kernels (Vulkan/Metal/raw OpenCL) are explicitly out of scope unless profiling later
  shows the CLAHE chain break dominates (risk R-10).
