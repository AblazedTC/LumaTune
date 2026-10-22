# ADR-0003: ONNX Runtime for inference (DR-3)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §3 DR-3

## Decision

ONNX Runtime **1.31.0** (released 2026-10-09) for V4's learned inference. Execution providers
targeted: CPU (always), **DirectML** (Windows GPU), **CoreML** (macOS, preview in ORT — gated
behind a setting), CUDA (Linux, optional build). ROCm is deprecated in ORT and not targeted.

Installed via vcpkg port `onnxruntime` where it is in sync with 1.31.0. ⚠️ No prebuilt `osx-x64`
archive is published upstream for 1.31.0, so Intel-Mac builds must compile ORT from vcpkg (see
ADR-0010 for the full per-platform sourcing story).

## Consequences

- Inference is CPU-first with GPU as an opportunistic speedup; the app must work correctly with
  ORT's CPU EP alone on any platform.
- `Ort::Session::Run` is never called directly — always through `OrtSession::run()`, serialized
  (DirectML requires single-threaded `Run` per session).
- Intel-Mac CI/build time is worse than Windows/arm64-Mac/Linux because ORT must be built from
  source there.
