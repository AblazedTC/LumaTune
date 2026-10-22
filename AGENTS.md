# Build guide for coding agents and contributors

This file is the single always-on instruction set for anyone — human or AI — working in this repository. Read it before changing anything. For *what* we are building and *why*, see [PLAN.md](./PLAN.md); this file covers *how* to work here.

> If you are an AI coding agent: treat every rule below as a hard constraint unless the user explicitly overrides it in the current conversation. When in doubt, ask; do not guess at architecture.

---

## 1. Project in one paragraph

**LumaTune** is a native desktop photo enhancer. A C++20 core (`luma_core`) runs a classical pipeline — linearise → white balance → sRGB encode → Lab auto-levels → CLAHE (own float implementation) → curve → saturation — in float32, with OpenCV's T-API for optional GPU colour conversion/resizing, RAW decoding (LibRaw) and learned stages (ONNX Runtime) layered on in later versions. The UI is Qt 6 Quick/QML with thin C++ backends. Python exists **only** for ML training/export and for parity tests through a nanobind module; it is never shipped. The original Python/PySide6 V1 lives in `legacy/` as a reference oracle until the C++ goldens become byte-exact (PLAN.md task P3.8), then it is deleted.

---

## 2. Repository map

| Path | What | Language | Ships? |
|---|---|---|---|
| `src/core/` | `luma_core` — image buffers, io, pipeline engine, stages | C++20 | ✅ |
| `src/ui/` | `luma_ui` — QObject backends + QML | C++20 / QML | ✅ |
| `src/app/` | `lumatune` executable, resources, platform files | C++20 | ✅ |
| `bindings/python/` | `luma_py` nanobind module (dev/test only) | C++20 | ❌ |
| `models/` | shipped `.onnx` + `manifest.json` (git LFS) | — | ✅ |
| `ml/` | PyTorch training, export, eval | Python ≥ 3.10 | ❌ |
| `tools/` | helper scripts (golden generation, notices, checks) | Python / CMake | ❌ |
| `tests/` | unit, golden, parity, ui, bench | C++ / Python | ❌ |
| `legacy/` | V1 PySide6 app (frozen oracle) | Python ≥ 3.9 | ❌ |
| `docs/` | ARCHITECTURE, COLOR, STAGES, MODELS, RELEASE, ADR/ | Markdown | ❌ |

Section 10 covers working on it.

---

## 3. Prerequisites

### All platforms
- **CMake ≥ 3.28**, **Ninja**, **git** with **git-lfs** (`git lfs install` once).
- **vcpkg** cloned anywhere; export `VCPKG_ROOT`. The manifest (`vcpkg.json`) installs OpenCV (with OpenCL), LibRaw, lcms2, libexif, spdlog, nlohmann-json, tl-expected, Catch2, Google Benchmark automatically on first configure. **ONNX Runtime is sourced per platform** (PLAN.md DR-10): official DirectML package on Windows, official arm64 archive on macOS, vcpkg overlay port on Intel Mac, vcpkg on Linux — CMake handles this; do not add `onnxruntime` to the manifest for Windows/macOS.
- **Qt 6.11.x open-source** (exact patch pinned in `cmake/Qt.cmake`). Install with the official online installer **or** `pip install aqtinstall && aqt install-qt <host> desktop 6.11.x <arch> -m qtshadertools qtimageformats`. Export `QT_ROOT` to the kit dir (e.g. `.../6.11.2/msvc2022_64`). **Do not** install Qt via vcpkg; **do not** use Qt 6.12 LTS (commercial-only).
- **Python ≥ 3.10** only if you build `luma_py` or work in `ml/`/`tools/`.

### Windows x64
- Visual Studio 2022 (17.10+) with "Desktop development with C++" (MSVC 19.4x), Windows 10/11 SDK.
- Run builds from a **"x64 Native Tools" prompt** or let the CMake preset find MSVC.

### macOS (arm64 or x64)
- Xcode 16+ with command-line tools (`xcode-select --install`).
- For release signing: Developer ID certificate, `notarytool` credentials in keychain (see `docs/RELEASE.md`).

### Linux x64
- GCC 13+ or Clang 17+, plus system packages Qt needs at runtime/development: `libgl1-mesa-dev libxkbcommon-dev libxcb-*-dev libfontconfig1-dev libfreetype6-dev libwayland-dev` (Ubuntu 22.04 names). `ocl-icd-opencl-dev` for OpenCL.

---

## 4. Build, run, test — the commands

Presets are defined in `CMakePresets.json`. Use them; do not hand-roll `-D` flags unless you are editing the presets.

```bash
# Configure + build (pick your preset)
cmake --preset win-msvc-release        # or mac-arm64-release | mac-x64-release | linux-gcc-release
cmake --build --preset win-msvc-release

# Run the app
./build/win-msvc-release/src/app/lumatune            # .exe on Windows; LumaTune.app/Contents/MacOS/LumaTune on macOS

# Tests (ctest labels: unit, golden, ui, bench, parity)
ctest --preset win-msvc-release -L unit
ctest --preset win-msvc-release -L golden
ctest --preset linux-gcc-release -L ui      # sets QT_QPA_PLATFORM=offscreen itself

# Benchmarks (not run by default ctest)
./build/<preset>/tests/bench/luma_bench --benchmark_filter=Pipeline

# Python bindings + parity tests
cmake --preset linux-gcc-release -DLUMA_BUILD_PY=ON && cmake --build --preset linux-gcc-release
python -m pip install -e bindings/python && pytest tests/parity -v

# Sanitizer builds (Linux/Clang)
cmake --preset linux-clang-asan && cmake --build --preset linux-clang-asan && ctest --preset linux-clang-asan

# Formatting / lint (also run in CI)
cmake --build --preset <preset> --target format-check   # clang-format + qmlformat
cmake --build --preset <preset> --target lint           # clang-tidy + qmllint + tools/check_includes.py
```

CMake options: `LUMA_BUILD_TESTS` (ON), `LUMA_BUILD_BENCH` (ON), `LUMA_BUILD_PY` (OFF), `LUMA_WITH_CUDA` (OFF, Linux), `LUMA_WITH_COREML` (ON on macOS, runtime-gated), `LUMA_ENABLE_LTO` (ON for release).

### Smallest command that validates your change
- Touched a stage in `src/core/stages/**` → `ctest -L unit -R <StageName>` then `ctest -L golden`.
- Touched `src/core/engine/**` → `ctest -L unit -R Engine` + a TSan run if threading changed.
- Touched `src/ui/**` → `ctest -L ui` and launch the app once with a fixture: `lumatune tests/golden/fixtures/warm_cast.png`.
- Touched QML only → `qmllint` + `qmlformat` via `--target lint` and a manual launch.
- Touched `ml/**` → `pytest ml/tests` and, if a model was re-exported, `python ml/export/verify_onnx.py`.
- Docs only → nothing to run.

---

## 5. Architecture rules (enforced; violating these fails CI)

1. **Layering:** `lumatune → luma_ui → luma_core → third-party`. `luma_core` **never includes a Qt header** (`tools/check_includes.py` greps for `#include <Q`). `luma_ui` never does pixel math — it marshals data and calls the engine.
2. **Stages are pure.** A `Stage::run` is a function of `(ImageBuffer, Params)`. No file IO, no globals, no thread creation inside a stage (OpenCV's own parallelism is fine). Check the `CancelToken` between coarse steps.
3. **The engine owns threads.** All `luma_core` work runs on `JobScheduler` worker threads. The UI thread only submits jobs and receives results via `Qt::QueuedConnection`. Never call `Pipeline::run` on the UI thread.
4. **Generation and job-kind discipline.** PREVIEW jobs go through the per-document latest-only slot; FULL jobs are cancelled by newer previews; EXPORT/BATCH jobs take immutable `(source, Params)` snapshots and are never cancelled by slider activity. Consumers drop PREVIEW/FULL results with `generation < latestSubmitted`. Never bypass this to "just show the result".
5. **Errors are values.** Core APIs return `tl::expected<T, Error>`. Exceptions are caught at the third-party boundary (OpenCV, LibRaw, ORT) and converted. No exception crosses the `luma_core` public API.
6. **Typed diagnostics.** Add a field to the relevant `*Diag` struct; never stuff values into a string map. UI and tests depend on the types.
7. **No GPL dependencies.** Qt Charts, Qt Graphs, Qt Quick 3D, Qt Quick Timeline, Qt Virtual Keyboard, qmlsc, exiv2 — all forbidden. Qt is linked **dynamically only** (LGPLv3). If you need a new library, check its license first and add it to `THIRD_PARTY_NOTICES.md`.
8. **No network, no telemetry.** The app never opens a socket.
9. **ORT `Run` is serialised per session** (DirectML requires it). Never call `Ort::Session::Run` without going through `OrtSession::run()`.

---

## 6. Image data contract

Agents regularly break these; read twice.

| Rule | Detail |
|---|---|
| Canonical format | `ImageBuffer{PixelFormat::RGB_F32}` interleaved, **RGB order** (never BGR outside the `imread`/`imwrite` call site), 64-byte aligned, immutable once published. Storage may be host (`HostStorage`) or OpenCL-resident (`DeviceStorage`); call `.mat()` / `.umat()` and let the engine decide where data lives. |
| Range | `[0, 1]`. No current decoder produces > 1.0 (LibRaw output is bounded), but stages must not *assume* clamped input. Clamp only where the maths requires it (`Encode`, final output). |
| Transfer-domain contract | Every edge in the graph has an explicit `Transfer` (PLAN.md §7.2). `ToLinear` → WB runs on **Linear**; `Encode` applies the sRGB OETF **exactly once**; everything from `ToLab` to `FromLab` is Lab; `FromLab` yields sRGB-encoded RGB — **do not apply an OETF after it**. Stages return `Error::WrongTransfer` if fed the wrong domain. |
| Colour space tags | Every buffer carries `ColorSpace{primaries, transfer, icc}`. ICC-tagged inputs are converted to sRGB primaries in `ToLinear` via lcms2. |
| Lab | Transient inside tone stages only; never stored in an `ImageBuffer`. `cv::COLOR_RGB2Lab` on CV_32F expects **sRGB-encoded** input in `[0,1]` and yields L ∈ [0,100]. |
| CLAHE | **Own implementation** (`luma::Clahe`, PLAN.md DR-9) on float L with `bins` (default 1024; 256 = V1 parity). Do **not** call `cv::createCLAHE` in production code — it is 8/16-bit only, its 16-bit clip semantics differ, and its OpenCL path is 8-bit only. |
| Histograms / readouts | 256-bin `float` arrays; black/white points reported on the **0–255 scale** for display continuity with V1. |
| Preview proxy | ≤ 1.5 MP, longest edge ≤ 1600, `cv::INTER_AREA`. Percentiles from the proxy are used for preview only; FULL jobs recompute. |
| Allocation | Use `BufferPool::acquire(w, h, format, device?)`; never `new`/`malloc` a pixel buffer in a stage. |
| UMat | Write against `cv::InputArray/OutputArray`. If `supportsUMat()` returns true, the stage must produce identical results (≤ 1e-3) on CPU and GPU — the golden tests run both. A stage that returns host storage after device input is a "chain break" and must be justified in its docstring. |
| RAW | WB gains and exposure for RAW documents are applied **inside LibRaw** (`user_mul`, `exp_shift`) in pass B; the engine's WB/Exposure slots are pass-through for RAW and only fill diagnostics. Never re-apply gains to LibRaw output. |

---

## 7. C++ conventions

- **Standard:** C++20. Use `std::span`, `std::string_view`, designated initialisers, `<format>`, `std::jthread`/`stop_token`. No `std::expected` yet (Apple Clang) — use `luma::Expected<T>` (alias of `tl::expected`).
- **Naming:** `PascalCase` types, `camelCase` functions and variables, `m_` prefix for private members, `k` prefix for constants (`kMaxPreviewPixels`), `UPPER_SNAKE` only for macros (avoid macros). Files match the primary type: `JobScheduler.h/.cpp`.
- **Headers:** `#pragma once`; include what you use (`misc-include-cleaner` runs in lint); forward-declare in headers where possible; keep OpenCV/LibRaw/ORT headers **out of public core headers** (use pimpl or opaque handles) so `luma_ui` compiles fast and stays decoupled.
- **Ownership:** `std::unique_ptr` by default, `std::shared_ptr<const T>` for results crossing threads, raw pointers/references for non-owning parameters. No `new`/`delete`.
- **Const & noexcept:** mark everything that can be. Stages are `const` and reentrant.
- **No exceptions in core control flow** (see rule 5). Third-party exceptions are caught in the adapter that calls them.
- **Logging:** `LUMA_LOG_DEBUG/INFO/WARN/ERROR(fmt, ...)` (spdlog). Never log pixel data; log dimensions/params/timings.
- **Comments:** explain *why*, not *what*. Public functions carry a one-line contract stating dtype, range, shape, and threading expectations.
- **Formatting:** `.clang-format` is law (LLVM base, 100 cols, 4-space indent, braces on same line). Run `--target format` before committing.
- **Warnings are errors in CI** (`/W4 /permissive-` or `-Wall -Wextra -Wpedantic -Wconversion`).

---

## 8. QML / Qt conventions

- QML files are **thin**: layout, bindings, and visual states only. Logic belongs in the C++ backend `QObject`s in `src/ui/`. If a QML file grows a JavaScript function longer than ~10 lines, move it to C++.
- Expose backends with `QML_ELEMENT`/`QML_SINGLETON` and `Q_PROPERTY(... NOTIFY ...)`; never use context properties.
- All user-visible strings go through `qsTr()`.
- Every interactive item sets `Accessible.name`; keyboard focus order is explicit via `KeyNavigation` or `activeFocusOnTab`.
- Sliders are integer-valued in QML; the int→float mapping lives in **one** place per parameter (`PipelineController::paramsFromUi`) with a comment stating the mapping (`0–50 → 0.0–5.0 %`). Mirror the inverse mapping right beside it.
- Render thread code (`updatePaintNode`) must not touch `QObject` state outside the documented double buffer.
- **Never** add Qt Charts/Graphs imports (GPL). Histogram and curve widgets are `QQuickPaintedItem`/`QSGGeometryNode` based.
- `qmlformat` + `qmllint` must pass.

---

## 9. Tests, goldens, and benchmarks

- Framework: **Catch2 v3**. One test file per source file (`tests/unit/stages/WhiteBalanceTest.cpp`). Use `TEST_CASE("WhiteBalance/GrayWorld neutral image yields unit gains", "[unit][wb]")` style names and tags.
- **Deterministic inputs**: synthetic images via `tests/support/Synthetic.h` (seeded `std::mt19937_64{42}`), or fixtures from `tests/golden/fixtures/`. No network, no temp-file dependence unless testing IO (then use `tests/support/TempDir`).
- **Golden tests**: fixtures and expected outputs are committed. If your change *intentionally* changes output, regenerate with `cmake --build --target regen-goldens`, inspect the diff images in `build/<preset>/golden-diff/`, and explain the visual change in the PR. Never regenerate goldens to make a failing test pass without that explanation.
- **Parity tests** (`tests/parity/`, pytest) compare `luma_py` against `legacy/` (until Phase 3) and PyTorch against ORT (V4). Tolerances are documented in each test; do not loosen them silently.
- **Benchmarks** use Google Benchmark; add one when you add a stage. CI flags > 15 % regressions vs `main`.
- **UI tests** run with `QT_QPA_PLATFORM=offscreen`; controllers are tested with `QTest`/`QSignalSpy`, QML with Qt Quick Test.
- A PR that adds a stage **must** add: unit test, golden set entry, benchmark, `docs/STAGES.md` row, and typed diagnostics.

---

## 10. Working on the legacy Python V1 (until Phase 3)

The V1 app is the oracle — change it **only** to fix a bug that would corrupt the golden set, and regenerate goldens afterwards with `tools/gen_golden.py`. It is deleted in P3.8 once the C++ goldens are byte-exact.

```bash
cd legacy
python -m venv .venv && .venv\Scripts\activate   # source .venv/bin/activate on macOS/Linux
pip install -r requirements.txt
python -m pytest tests/ -v
python app.py
```

Conventions that still apply there: RGB float32 `[0,1]` in memory; WB on RGB, levels/CLAHE on Lab L only; all processing on `QThreadPool`, never the UI thread; slider int→float mapping only in `MainWindow._current_params`; `from __future__ import annotations` for 3.9 compatibility; tests use seeded synthetic arrays.

---

## 11. Working in `ml/` (V4)

- `cd ml && python -m venv .venv && pip install -e .[dev]` (Python ≥ 3.10, PyTorch, Lightning, ONNX, onnxruntime, hydra).
- Configs are Hydra YAML in `ml/configs/`; every run writes `config.yaml`, `git_sha`, dataset hashes, and metrics next to checkpoints. **No run without a config.**
- **Licensing gate:** a model may only be copied into `models/` if its `manifest.json` entry has `training_data_license` ∈ {CC0, CC-BY-4.0, own} **and** its `checkpoint_ancestry` contains no run tagged `research-only`. FiveK and PPR10K are **non-commercial** — use them only for architecture/hyper-parameter selection; shipped models train from random init on permissively licensed data. No weight transfer from research runs, not even the backbone. Release CI enforces both checks.
- Export with `ml/export/export_onnx.py` (opset 18, fixed shapes) and verify with `verify_onnx.py` (PyTorch↔ORT ≤ 1e-4 fp32, EP coverage). Commit `.onnx` via git LFS.
- Model outputs are **parameters** (illuminant log-chroma for WbNet; `Params` v2 fields — curve knots, clip/tile, saturation, levels — for ToneNet), never pixels. WbNet predicts the *illuminant*; gains are derived outside the model. If you find yourself writing an image-to-image head, stop and re-read PLAN.md §17.

---

## 12. Dependency changes

- Add to `vcpkg.json` (with a version constraint if needed) and run configure; commit the manifest. Never commit `vcpkg_installed/`.
- Qt version bumps: edit `cmake/Qt.cmake` + CI workflow + `docs/RELEASE.md`; run full CI matrix.
- Any new third-party code → license check (§5 rule 7) → entry in `THIRD_PARTY_NOTICES.md` via `tools/gen_notices.py`.
- Python deps: `ml/pyproject.toml` or `tools/requirements.txt`; never the root.

---

## 13. Git workflow

- Branches: `feat/<area>-<short>`, `fix/<area>-<short>`, `chore/…`, `docs/…`. Area ∈ {core, ui, app, io, engine, stage, ml, build, ci}.
- Conventional Commits: `feat(engine): add generation-based job cancellation`. Reference PLAN task IDs: `(P1.7)`.
- Small PRs (< 500 lines diff excluding goldens). One task ID per PR where possible.
- PR checklist (copied into the PR template):
  - [ ] Builds on all four presets (CI)
  - [ ] Unit + golden tests green; goldens regenerated **only** with explanation
  - [ ] clang-format / clang-tidy / qmllint clean
  - [ ] No Qt include in `src/core`; no new GPL dependency
  - [ ] Typed diagnostics + docs updated if a stage/param changed
  - [ ] Benchmark added/updated if perf-relevant
  - [ ] PLAN.md task marked or amended if scope changed
- Agents: include a `Co-authored-by:` trailer for your identity; never force-push shared branches; never commit secrets, signing keys, or dataset files.

---

## 14. Things agents must NOT do

- Do not add Python to the runtime path or make the app depend on a Python interpreter.
- Do not statically link Qt or LibRaw.
- Do not introduce `std::map<std::string, std::any>`-style diagnostics or JSON blobs as internal data contracts.
- Do not call OpenCV with BGR assumptions on our RGB buffers.
- Do not apply the sRGB OETF anywhere except the `Encode` stage; do not feed linear data to `ToLab`.
- Do not use `cv::createCLAHE` in production code; use `luma::Clahe`.
- Do not quantise to 8-bit anywhere before the final output.
- Do not call `Ort::Session::Run` directly; use `OrtSession::run()` (serialised).
- Do not run pipeline work on the UI thread "just for a quick test".
- Do not regenerate goldens to silence a failure.
- Do not download datasets or models in CI or at app runtime.
- Do not create planning/notes Markdown files outside `docs/` and `PLAN.md`; keep scratch work out of the repo.
- Do not edit `legacy/` except to fix oracle-corrupting bugs (§10).

---

## 15. Where to look

| Question | Answer |
|---|---|
| Why is it built this way? | `PLAN.md` §3–§9, `docs/ADR/` |
| What does each stage do, exactly? | `docs/STAGES.md`, `src/core/stages/**` |
| Colour-space maths, Lab/linear handling | `docs/COLOR.md` |
| How do I release? | `docs/RELEASE.md` |
| Model cards and metrics | `docs/MODELS.md`, `models/manifest.json` |
| Perf numbers | `docs/PERF.md`, CI bench artefacts |
| V1 user docs (legacy) | `legacy/README.md` |
