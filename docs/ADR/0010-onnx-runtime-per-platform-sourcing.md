# ADR-0010: ONNX Runtime sourced per platform, EP presence asserted at configure time (DR-10)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §3 DR-10, §10.2

## Decision

vcpkg's `onnxruntime` port exposes no DirectML or CoreML feature, so vcpkg alone yields CPU-only
inference on Windows/macOS. Instead:

- **Windows:** official DirectML package (`Microsoft.ML.OnnxRuntime.DirectML`) via CMake
  `FetchContent`.
- **macOS arm64:** official `onnxruntime-osx-arm64` archive (CoreML EP included).
- **macOS x64 (Intel):** vcpkg `onnxruntime` built from source via an overlay port with CoreML
  enabled (no upstream prebuilt for Intel Mac at 1.31.0).
- **Linux:** vcpkg `onnxruntime` (+ optional `cuda` feature).

CMake fails configuration if the expected EP symbol is absent
(`check_cxx_symbol_exists(OrtSessionOptionsAppendExecutionProvider_DML ...)` / the CoreML
equivalent), and packaging fails if the EP library is missing from the bundle.

## Consequences

- Four different sourcing paths for one dependency — more CMake complexity than a single vcpkg
  line, but it's the only way to get GPU EPs on Windows/macOS at all.
- A silent CPU-only fallback is explicitly not acceptable: missing EP = build/package failure,
  not a degraded-but-working build.
- Intel Mac CI is the slowest leg of the matrix (building ORT from source).
