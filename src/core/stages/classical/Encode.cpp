#include "Encode.h"

#include <opencv2/core.hpp>

#include "../../image/BufferPool.h"
#include "../../image/Transforms.h"

namespace luma {

ImageBuffer encodeStage(const ImageBuffer& input) {
    ImageBuffer output = BufferPool::instance().acquire(input.width, input.height, input.format, input.colorSpace);
    input.mat().copyTo(output.mat());
    output.exifOrientation = input.exifOrientation;

    cv::Mat mat = output.mat();
    toSrgb(mat);
    cv::min(mat, 1.0, mat);
    cv::max(mat, 0.0, mat);

    output.colorSpace.transfer = Transfer::sRGB;
    return output;
}

} // namespace luma
