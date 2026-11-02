#pragma once

#include <opencv2/core.hpp>

#include "../../engine/Params.h"

namespace luma {

// Scales Lab a/b about neutral (0) by params.scale; L passes through
// unchanged (PLAN.md §7.4). New in the rewrite (bypass defaults true) -
// no legacy behaviour to match. `lab` must be CV_32FC3 (CV_Assert'd; see
// AutoLevels.h for why this isn't a recoverable Expected-returning check).
cv::Mat saturation(const cv::Mat& lab, const SaturationParams& params);

} // namespace luma
