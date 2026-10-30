#include "ImageBuffer.h"

#include <cassert>

namespace luma {

int toCvType(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGB_F32:
            return CV_32FC3;
        case PixelFormat::RGBA_F32:
            return CV_32FC4;
        case PixelFormat::GRAY_F32:
            return CV_32FC1;
        case PixelFormat::GRAY_U16:
            return CV_16UC1;
        case PixelFormat::RGB_U16:
            return CV_16UC3;
        case PixelFormat::RGB_U8:
            return CV_8UC3;
    }
    assert(false && "unhandled PixelFormat in toCvType");
    return -1;
}

cv::Mat ImageBuffer::mat() const {
    if (const auto* host = std::get_if<HostStorage>(&storage)) {
        // Non-owning view: HostStorage.data stays the owner (shared_ptr).
        // strideBytes == 0 means "tightly packed", which is exactly what
        // cv::Mat::AUTO_STEP (0) means too, so this passes through as-is.
        return cv::Mat(height, width, toCvType(format), host->data.get(), host->strideBytes);
    }
    const auto& device = std::get<DeviceStorage>(storage);
    return device.umat.getMat(cv::ACCESS_READ).clone();
}

cv::UMat ImageBuffer::umat() const {
    if (const auto* device = std::get_if<DeviceStorage>(&storage)) {
        return device->umat;
    }
    cv::UMat result;
    mat().copyTo(result);
    return result;
}

} // namespace luma
