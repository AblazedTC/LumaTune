# ADR-0001: C++ runtime, Python research tooling only (DR-1)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §3 DR-1

## Decision

Everything shipped to users is C++20: the core processing library (`luma_core`), RAW decoding,
ONNX inference, and the Qt Quick UI host. Python is retained **only** in `tools/` and `ml/` for
dataset preparation, PyTorch training, ONNX export, and parity testing against the C++ core
through a nanobind module. Python is never bundled in a release.

C++ standard: C++20 (concepts, `std::span`, designated initializers, `<format>`). Not C++23 —
Apple Clang lags on `std::expected`; `tl::expected` is used until all three toolchains ship it.

## Alternatives considered

| Option | Verdict | Why |
|---|---|---|
| Keep PySide6 UI, C++ core via bindings | Rejected | Still ships a ~100 MB Python runtime; two languages in the runtime path; packaging pain on macOS notarization — fails goal G-1 (no Python at runtime) |
| Full C++ including training | Rejected | Training ecosystem in C++ is far weaker; slows V4 badly |
| Rust | Rejected | Qt Quick bindings immature; OpenCV/LibRaw/ORT bindings thinner; retrains the whole team |

## Consequences

- Two toolchains exist in the repo, but Python is contained to `ml/` and `tools/`; `luma_core`
  and everything it links against must stay Qt-free and Python-free.
- Training-stack quality (PyTorch/Lightning) is kept without paying for it at runtime.
- The legacy V1 app (Python/PySide6) is the transitional case: it ships nothing today either,
  and is deleted once the C++ goldens are byte-exact (P3.8).
