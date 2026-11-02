#pragma once

#include <opencv2/core.hpp>

#include "../../engine/Diagnostics.h"
#include "../../engine/Params.h"

namespace luma {

struct ClaheResult {
    cv::Mat lab; // CV_32FC3, L in [0,100]
    ClaheDiag diag;
};

// Own float CLAHE (PLAN.md DR-9 - cv::CLAHE is 8/16-bit only and its
// OpenCL path is 8-bit only, neither matches our float/GPU-parity needs).
// Per tile: a `bins`-bucket histogram of L spanning [0,100], clipped at
// `clipLimit * tilePixels / bins` (OpenCV's own 8-bit clip formula) with
// the excess redistributed uniformly across all bins, then a cumulative
// sum rescaled to [0,100] as that tile's lookup table. Each output pixel
// bilinearly blends the LUT value of its own bin from the four
// surrounding tiles (same tile-center interpolation geometry as
// cv::CLAHE's own `CLAHE_Interpolation_Body`). a/b pass through
// unchanged. Tiles are independent; built via `cv::parallel_for_`.
//
// `lab` must be CV_32FC3 (CV_Assert'd; see AutoLevels.h for why this
// isn't a recoverable Expected-returning check).
//
// `bins=256` reproduces OpenCV's own 8-bit `cv::CLAHE` (legacy's
// `cv2.createCLAHE`, which this is parity-tested against directly) on
// uint8-quantised input within 1 LSB - see ClaheTest.cpp. Image
// dimensions that aren't an exact multiple of `tilesX`/`tilesY` get
// smaller (not padded) edge tiles, which is a deliberate simplification
// and diverges from OpenCV's own border-replication padding for that
// case; this is untested against OpenCV for non-exact-multiple sizes.
ClaheResult clahe(const cv::Mat& lab, const ClaheParams& params);

} // namespace luma
