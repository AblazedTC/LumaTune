# LumaTune V1 (legacy, frozen)

This is the original Python/PySide6 app. It is kept only as a reference oracle for golden-image parity testing during the C++ rewrite (see [../PLAN.md](../PLAN.md)). Do not add features here — fix bugs only if they corrupt the golden set, then regenerate goldens with `tools/gen_golden.py`. It is deleted once the C++ goldens are byte-exact (PLAN.md task P3.8).

A desktop app that loads a photo and runs a set of classical image processing algorithms on it. The output looks better exposed and more balanced than the original.

It works on JPEG, PNG, BMP, TIFF, and WebP files. Images are loaded as 8-bit (16-bit sources are downcast) and EXIF orientation is not applied. RAW is not supported here (V2 of the rewrite).

---

## What it does

When you open an image, the app runs three steps in order:

1. **White balance** - estimates the color of the light in the scene and corrects for it using the Gray World algorithm.
2. **Auto levels** - finds the actual dark and bright points in the image and stretches the tonal range to fill the full output range.
3. **Local contrast** - applies CLAHE to bring out detail in shadows and highlights without blowing out the bright areas.

White balance runs on the RGB image. The image is then converted to Lab, and auto levels and CLAHE adjust only the lightness channel, so colors shift as little as possible beyond the white balance correction.

The app shows a before and after preview side by side, a histogram comparing the luminance before and after, and a readout of what the algorithm decided.

---

## Requirements

- Python 3.9 or newer
- Windows, macOS, or Linux with a display

---

## Setup

Run from inside `legacy/`. Create and activate the virtual environment:

```bash
python -m venv .venv

# Windows
.venv\Scripts\activate

# macOS / Linux
source .venv/bin/activate
```

Install dependencies:

```bash
pip install -r requirements.txt
```

---

## Run

```bash
python app.py
```

Click **Open Image** to load a file. The pipeline runs automatically. Click **Export** to save the result as PNG or JPEG. Metadata (EXIF, ICC) is not preserved on export.

---

## Sliders

| Slider     | What it changes                                                                 | Default |
| ---------- | ------------------------------------------------------------------------------- | ------- |
| Shadows    | The percentile used to pick the black point (0 to 5)                            | 0.5     |
| Highlights | The percentile used to pick the white point (95 to 100)                         | 99.5    |
| Clarity    | CLAHE clip limit, controls how aggressively local contrast is boosted (1 to 10) | 2.0     |
| Tile Size  | CLAHE tile grid size in pixels (4 to 16)                                        | 8       |

Adjusting any slider re-runs the pipeline automatically with a short delay.

---

## Tests

```bash
python -m pytest tests/ -v
```

Tests cover the white balance, auto levels, and end-to-end pipeline modules. The CLAHE wrapper, image loading, and UI are not yet covered.

---

## Project structure

```
app.py                  entry point
models/
  image_state.py        holds the loaded image data
processing/
  white_balance.py      Gray World algorithm
  levels.py             percentile black/white point estimation and stretch
  tone_curve.py         CLAHE wrapper
  pipeline.py           runs the three steps in order
ui/
  main_window.py        main window, layout, and signal wiring
  histogram_widget.py   draws the before/after luminance histogram
tests/
  test_white_balance.py
  test_levels.py
  test_pipeline.py
```
