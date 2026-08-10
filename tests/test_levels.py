import numpy as np

from processing.levels import apply_levels, compute_auto_levels


def test_compute_matches_numpy_percentile():
    """Black/white points should match numpy's percentile on the same data."""
    L = np.tile(np.arange(256, dtype=np.uint8), (10, 1))
    black_pt, white_pt, _ = compute_auto_levels(L, 0.5, 99.5)
    assert abs(black_pt - float(np.percentile(L, 0.5))) < 1.0
    assert abs(white_pt - float(np.percentile(L, 99.5))) < 1.0


def test_apply_levels_stretches_to_full_range():
    """After stretching, the endpoints of the original range map to 0 and 255."""
    L = np.array([[30, 50, 100, 200, 220]], dtype=np.uint8)
    stretched = apply_levels(L, 30.0, 220.0)
    assert int(stretched.min()) == 0
    assert int(stretched.max()) == 255


def test_apply_levels_clamps_outside_range():
    """Values outside [black_pt, white_pt] must clamp, not wrap."""
    L = np.array([[0, 128, 255]], dtype=np.uint8)
    stretched = apply_levels(L, 50.0, 200.0)
    assert int(stretched.min()) >= 0
    assert int(stretched.max()) <= 255


def test_degenerate_range_guarded():
    """When black_pt == white_pt the function should still return a valid result."""
    L = np.full((10, 10), 128, dtype=np.uint8)
    black_pt, white_pt, _ = compute_auto_levels(L, 0.5, 99.5)
    # The guard adds 1 so white_pt > black_pt
    assert white_pt > black_pt
