# Image Enhancement Pipeline

A desktop app that loads a photo and runs a set of classical image processing algorithms on it. The output looks better exposed and more balanced than the original.

It works on JPEG, PNG, BMP, TIFF, and WebP files. Images are loaded as 8-bit (16-bit sources are downcast) and EXIF orientation is not applied yet. RAW support is planned for a later version.

The original Python/PySide6 app (what it does, setup, running it, sliders, tests) now lives in [`legacy/`](./legacy) as a frozen reference oracle for the rewrite — see [legacy/README.md](./legacy/README.md) to run it.

---

## Roadmap

This Python app is **V1** and is being rewritten as a native C++ / Qt Quick executable. The full engineering plan — architecture, phases, and the V2–V4 feature designs — is in [PLAN.md](./PLAN.md). Contributor and coding-agent conventions are in [AGENTS.md](./AGENTS.md).

- **V2** - RAW file support via LibRaw, linear-light processing
- **V3** - Adaptive algorithm selection based on scene features, with explanations
- **V4** - Learned white balance and tone mapping (ONNX Runtime), with classical fallback
