#include "ToLinear.h"

#include <opencv2/imgproc.hpp>

#include "../../image/BufferPool.h"

namespace luma {

Expected<ImageBuffer, ColorError> toLinearStage(const ImageBuffer& input) {
    ImageBuffer output =
        BufferPool::instance().acquire(input.width, input.height, input.format, input.colorSpace);
    input.mat().copyTo(output.mat());
    output.exifOrientation = input.exifOrientation;

    cv::Mat mat = output.mat();

    if (output.colorSpace.icc.has_value()) {
        auto result = iccToSrgb(mat, *output.colorSpace.icc);
        if (!result.has_value()) {
            return make_unexpected(result.error());
        }
        toLinear(mat); // iccToSrgb leaves it sRGB-encoded; now apply the sRGB EOTF
    } else if (output.colorSpace.primaries == Primaries::AdobeRGB ||
               output.colorSpace.primaries == Primaries::ProPhoto ||
               output.colorSpace.primaries == Primaries::CameraNative) {
        // Non-sRGB primaries with no ICC profile to convert them via: this
        // stage has no standalone primaries-conversion matrix (only
        // Transforms::iccToSrgb, which needs real ICC bytes). No current
        // caller produces this (StdDecoder only ever sets sRGB or
        // Unknown+icc) - fail loudly rather than silently mis-tag the
        // output as sRGB when it isn't.
        return make_unexpected(ColorError::UnsupportedPrimaries);
    } else if (output.colorSpace.transfer == Transfer::Linear) {
        // already linear - nothing to do
    } else if (output.colorSpace.transfer == Transfer::Gamma22) {
        cv::pow(mat, 2.2, mat);
    } else {
        // Transfer::sRGB, or Transfer::Unknown without an ICC profile
        // (shouldn't happen given how StdDecoder sets ColorSpace, but
        // sRGB is the documented fallback assumption for untagged input).
        toLinear(mat);
    }

    output.colorSpace = ColorSpace{Primaries::sRGB, Transfer::Linear, std::nullopt};
    return output;
}

} // namespace luma
