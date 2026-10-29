#pragma once

#include <opencv2/core.hpp>

#include "ColorSpace.h"
#include "../util/Expected.h"

namespace luma {

enum class ColorError { IccParseFailed, IccTransformFailed };

// Per-channel sRGB OETF/EOTF (IEC 61966-2-1) on a single value in [0,1].
// Free functions so the math is unit-testable without OpenCV; round-trip
// error is ~2e-7 in float32, well inside PLAN.md's 1e-5 budget.
float srgbToLinear(float c);
float linearToSrgb(float c);

// In-place; `image` is RGB_F32/RGBA_F32/GRAY_F32. A 4th (alpha) channel is
// left untouched - alpha is linear by definition, never gamma-encoded.
void toLinear(cv::Mat& image);
void toSrgb(cv::Mat& image);

// cv::cvtColor wrappers (PLAN.md §7.4). `srgbRgb` is sRGB-encoded float32
// RGB in [0,1]; `lab` is float32 with L in [0,100].
cv::Mat rgbToLab(const cv::Mat& srgbRgb);
cv::Mat labToRgb(const cv::Mat& lab);

// Converts `image` (RGB_F32) from the primaries described by `icc` to sRGB
// primaries, in place, via lcms2. A no-op (success) if `icc.data` is empty.
Expected<void, ColorError> iccToSrgb(cv::Mat& image, const IccProfile& icc);

} // namespace luma
