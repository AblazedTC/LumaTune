import numpy as np

from models.image_state import ImageState
from processing.pipeline import Pipeline, ProcessingParams


def _synthetic_state(h: int = 64, w: int = 64) -> ImageState:
    rng = np.random.default_rng(42)
    rgb = rng.random((h, w, 3)).astype(np.float32)
    return ImageState(original_rgb=rgb, source_path="<synthetic>")


def test_output_shape_matches_input():
    state = _synthetic_state()
    output, _ = Pipeline.run(state, ProcessingParams())
    assert output.shape == (64, 64, 3)


def test_output_in_unit_range():
    state = _synthetic_state()
    output, _ = Pipeline.run(state, ProcessingParams())
    assert output.min() >= 0.0
    assert output.max() <= 1.0


def test_no_nan_in_output_or_histograms():
    state = _synthetic_state()
    output, diag = Pipeline.run(state, ProcessingParams())
    assert not np.any(np.isnan(output))
    assert not np.any(np.isnan(diag["hist_before"]))
    assert not np.any(np.isnan(diag["hist_after"]))


def test_diagnostics_has_required_keys():
    state = _synthetic_state()
    _, diag = Pipeline.run(state, ProcessingParams())
    for key in ("wb", "levels", "clahe", "hist_before", "hist_after"):
        assert key in diag


def test_histogram_bins_are_256():
    state = _synthetic_state()
    _, diag = Pipeline.run(state, ProcessingParams())
    assert len(diag["hist_before"]) == 256
    assert len(diag["hist_after"]) == 256


def test_high_shadow_pct_raises_black_point():
    """A higher shadow percentile should produce a higher black point."""
    state = _synthetic_state()
    _, diag_low = Pipeline.run(state, ProcessingParams(black_pct=0.5))
    _, diag_high = Pipeline.run(state, ProcessingParams(black_pct=5.0))
    assert diag_high["levels"]["black_point"] >= diag_low["levels"]["black_point"]
