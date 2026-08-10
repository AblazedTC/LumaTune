import numpy as np

from processing.white_balance import gray_world_wb


def test_neutral_image_multipliers_are_one():
    """A uniform gray image should produce multipliers ≈ 1.0."""
    gray = np.full((64, 64, 3), 0.5, dtype=np.float32)
    _, diag = gray_world_wb(gray)
    assert abs(diag["mult_r"] - 1.0) < 1e-5
    assert abs(diag["mult_g"] - 1.0) < 1e-5
    assert abs(diag["mult_b"] - 1.0) < 1e-5


def test_red_cast_reduces_red_channel():
    """An image dominated by red should have mult_r < mult_g < mult_b."""
    img = np.zeros((64, 64, 3), dtype=np.float32)
    img[:, :, 0] = 0.8
    img[:, :, 1] = 0.4
    img[:, :, 2] = 0.2
    _, diag = gray_world_wb(img)
    assert diag["mult_r"] < diag["mult_g"]
    assert diag["mult_g"] < diag["mult_b"]


def test_output_stays_in_unit_range():
    """Output must always be in [0, 1] even with an extreme color cast."""
    img = np.ones((32, 32, 3), dtype=np.float32)
    img[:, :, 1] = 0.01  # almost no green → mult_g will be very large
    corrected, _ = gray_world_wb(img)
    assert corrected.min() >= 0.0
    assert corrected.max() <= 1.0
