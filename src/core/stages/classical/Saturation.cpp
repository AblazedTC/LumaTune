#include "Saturation.h"

#include <vector>

namespace luma {

cv::Mat saturation(const cv::Mat& lab, const SaturationParams& params) {
    CV_Assert(lab.channels() == 3 && lab.depth() == CV_32F);
    if (params.bypass) {
        return lab.clone();
    }

    std::vector<cv::Mat> channels;
    cv::split(lab, channels);
    channels[1] *= static_cast<double>(params.scale);
    channels[2] *= static_cast<double>(params.scale);

    cv::Mat result;
    cv::merge(channels, result);
    return result;
}

} // namespace luma
