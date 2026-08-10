# Image Enhancement Pipeline

A desktop app that loads a photo and runs a set of classical image processing algorithms on it. The output looks better exposed and more balanced than the original.

It works on JPEG and PNG files. RAW support is planned for a later version.

---

## What it does

When you open an image, the app runs three steps in order:

1. **White balance** - estimates the color of the light in the scene and corrects for it using the Gray World algorithm.
2. **Auto levels** - finds the actual dark and bright points in the image and stretches the tonal range to fill the full output range.
3. **Local contrast** - applies CLAHE to bring out detail in shadows and highlights without blowing out the bright areas.

All three steps work in the Lab color space. Only the lightness channel is adjusted, so colors shift as little as possible.

The app shows a before and after preview side by side, a histogram comparing the luminance before and after, and a readout of what the algorithm decided.

---

## Requirements

- Python 3.9 or newer
- Windows, macOS, or Linux with a display

---

## Setup

Create and activate the virtual environment:

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

Click **Open Image** to load a file. The pipeline runs automatically. Click **Export** to save the result.

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

---

## Roadmap

- **V2** - RAW file support via rawpy, linear RGB processing
- **V3** - Adaptive algorithm selection based on scene classification
- **V4** - Learned white balance and tone mapping
