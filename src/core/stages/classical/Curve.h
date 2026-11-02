#pragma once

#include <opencv2/core.hpp>

#include "../../engine/Params.h"

namespace luma {

// Applies, in order, the 16-knot piecewise-linear L curve (knots are
// already-monotonic Y-values at implicit evenly-spaced X positions - see
// Params.h), then a midpoint-pivoted contrast adjustment, then a gamma
// curve, each clamped to [0,100] (PLAN.md §7.4/FR-9). a/b pass through
// unchanged. New in the rewrite (bypass defaults true) - no legacy
// behaviour to match. `lab` must be CV_32FC3 (CV_Assert'd; see AutoLevels.h
// for why this isn't a recoverable Expected-returning check).
cv::Mat curve(const cv::Mat& lab, const CurveParams& params);

} // namespace luma
