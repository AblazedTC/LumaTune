#pragma once

#include <opencv2/core.hpp>

#include "../../engine/Diagnostics.h"
#include "../../engine/Params.h"

namespace luma {

struct LevelsResult {
    cv::Mat lab; // CV_32FC3, L in [0,100]
    LevelsDiag diag;
};

// Stretches the L channel's [blackPt, whitePt] percentile range to [0,100],
// clamping outside it; a/b pass through unchanged (PLAN.md §7.4). Operates
// directly on Lab cv::Mat, not ImageBuffer - Lab is transient, never stored
// in an ImageBuffer (PLAN.md §6.1). `lab` must be CV_32FC3 (CV_Assert'd);
// unlike WhiteBalance's PixelFormat, there's no caller-controlled runtime
// tag to check here, so a wrong-shaped Mat is a programmer error, not a
// recoverable one.
//
// Percentiles are read off a 4096-bin histogram of L (PLAN.md §7.4: "fast,
// deterministic"), not a full sort - bin width is ~0.024 L-units, well
// inside the 0.5 L-unit legacy-parity budget (legacy/processing/levels.py's
// np.percentile, which this doesn't bit-match exactly, by design).
LevelsResult autoLevels(const cv::Mat& lab, const LevelsParams& params);

} // namespace luma
