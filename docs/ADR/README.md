# Architecture Decision Records

One ADR per `DR-n` decision record in [`PLAN.md`](../../PLAN.md). Numbering matches the `DR-n`
number directly. When a decision is revisited, PLAN.md's convention is to add a new DR rather than
edit the old one — ADRs follow the same rule: write a new file, don't rewrite an `Accepted` one.

| ADR | Decision | Status |
|---|---|---|
| [0001](./0001-cpp-runtime-python-tooling.md) | C++ runtime, Python research tooling only | Accepted |
| [0002](./0002-qt6-quick-qml-ui.md) | Qt 6 Quick/QML, open-source 6.11.x | Accepted |
| [0003](./0003-onnx-runtime-inference.md) | ONNX Runtime for inference | Accepted |
| [0004](./0004-opencv-t-api-opencl.md) | OpenCV 4.14 with T-API (UMat) | Accepted |
| [0005](./0005-libraw-for-raw.md) | LibRaw 0.22.2 for RAW | Accepted |
| [0006](./0006-opencv-t-api-device-chain-fallback.md) | T-API opt-in, device chain, auto fallback | Accepted |
| [0007](./0007-exif-icc-library-pending.md) | EXIF/ICC library choice | Pending (P1.2) |
| [0008](./0008-shipped-model-training-data-provenance.md) | Shipped-model training data provenance | Accepted |
| [0009](./0009-own-float-clahe.md) | Own float CLAHE, not `cv::CLAHE` | Accepted |
| [0010](./0010-onnx-runtime-per-platform-sourcing.md) | ONNX Runtime sourced per platform | Accepted |
| [0011](./0011-wbnet-native-linear-space-domain-flag.md) | WbNet native linear space + domain flag | Accepted |
