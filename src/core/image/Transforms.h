#pragma once

#include <opencv2/core.hpp>

#include "ColorSpace.h"
#include "../util/Expected.h"

namespace luma {

// ponytail: one error enum for "this input isn't valid for a colour/domain-
// sensitive operation", shared by Transforms, ToLinear, Encode, and
// WhiteBalance, rather than a StageError per stage - revisit if the set of
// stages needing this grows enough that the shared enum stops making sense.
enum class ColorError {
    IccParseFailed,
    IccTransformFailed,
    WrongTransfer,
    UnsupportedPrimaries,
    UnsupportedFormat
};

// Per-channel sRGB OETF/EOTF (IEC 61966-2-1) on a single value in [0,1].
// Free functions so the math is unit-testable without OpenCV; round-trip
// error is ~2e-7 in float32, well inside PLAN.md's 1e-5 budget.
float srgbToLinear(float c);
float linearToSrgb(float c);

// In-place, RGB_F32 (3-channel) only - PLAN.md DR-6's UMat overloads below
// are the reason: a composed-ops (compare+select, no per-pixel loop)
// implementation that works identically on cv::Mat and cv::UMat has no
// natural "skip the 4th channel" step the way the old per-pixel loop did,
// and nothing in the live pipeline currently produces RGBA_F32 anyway
// (ponytail: add that back if something actually needs it).
void toLinear(cv::Mat& image);
void toSrgb(cv::Mat& image);

// PLAN.md DR-6/P1.10: UMat overloads of the above, for the device-resident
// chain. Same composed-ops implementation (shared internally), verified
// to agree with the cv::Mat overload within 1e-3 (TransformsTest.cpp) -
// cv::compare/cv::Mat::copyTo(mask) both transparently support UMat, so
// this never touches host memory if `image` is already device-resident.
void toLinear(cv::UMat& image);
void toSrgb(cv::UMat& image);

// cv::cvtColor wrappers (PLAN.md §7.4). `srgbRgb` is sRGB-encoded float32
// RGB in [0,1]; `lab` is float32 with L in [0,100]. Only CV_8U/CV_32F are
// valid depths for either Mat - cv::cvtColor throws on CV_64F.
cv::Mat rgbToLab(const cv::Mat& srgbRgb);
cv::Mat labToRgb(const cv::Mat& lab);

// InputArray/OutputArray overloads of the above (PLAN.md DR-6/P1.10):
// cv::cvtColor already dispatches to OpenCL transparently based on the
// actual argument type, so these just avoid forcing a concrete cv::Mat
// the way the pair above does. `lab`/`rgb` are (re)allocated as whichever
// backend `srgbRgb`/`lab` actually is.
void rgbToLab(cv::InputArray srgbRgb, cv::OutputArray lab);
void labToRgb(cv::InputArray lab, cv::OutputArray rgb);

// Converts `image` (RGB_F32) from the primaries described by `icc` to sRGB
// primaries, in place, via lcms2. A no-op (success) if `icc.data` is empty.
Expected<void, ColorError> iccToSrgb(cv::Mat& image, const IccProfile& icc);

} // namespace luma
