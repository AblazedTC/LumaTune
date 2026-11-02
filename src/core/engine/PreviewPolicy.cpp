#include "PreviewPolicy.h"

#include <algorithm>
#include <cmath>

#include <opencv2/imgproc.hpp>

#include "../image/BufferPool.h"

namespace luma {

ImageBuffer buildPreviewProxy(const ImageBuffer& source) {
    const double pixelScale =
        (static_cast<double>(source.width) * source.height > PreviewPolicy::kMaxPixels)
            ? std::sqrt(static_cast<double>(PreviewPolicy::kMaxPixels) /
                        (static_cast<double>(source.width) * source.height))
            : 1.0;
    const int longestEdge = std::max(source.width, source.height);
    const double maxEdge = static_cast<double>(PreviewPolicy::kMaxLongestEdge);
    const double edgeScale = (longestEdge > PreviewPolicy::kMaxLongestEdge) ? maxEdge / longestEdge
                                                                             : 1.0;
    const double scale = std::min(pixelScale, edgeScale);

    const int newWidth = std::max(1, static_cast<int>(std::lround(source.width * scale)));
    const int newHeight = std::max(1, static_cast<int>(std::lround(source.height * scale)));

    ImageBuffer proxy =
        BufferPool::instance().acquire(newWidth, newHeight, source.format, source.colorSpace);
    if (newWidth == source.width && newHeight == source.height) {
        source.mat().copyTo(proxy.mat());
    } else {
        cv::resize(source.mat(), proxy.mat(), cv::Size(newWidth, newHeight), 0, 0, cv::INTER_AREA);
    }
    proxy.exifOrientation = source.exifOrientation;
    return proxy;
}

} // namespace luma
