#include "Transforms.h"

#include <cmath>

#include <lcms2.h>
#include <opencv2/imgproc.hpp>

namespace luma {

float srgbToLinear(float c) {
    return (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

float linearToSrgb(float c) {
    return (c <= 0.0031308f) ? (c * 12.92f) : (1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
}

namespace {

void applyTransferInPlace(cv::Mat& image, float (*transfer)(float)) {
    CV_Assert(image.depth() == CV_32F);
    const int channels = image.channels();
    const int colorChannels = (channels == 4) ? 3 : channels; // alpha is always linear
    for (int row = 0; row < image.rows; ++row) {
        float* pixel = image.ptr<float>(row);
        for (int col = 0; col < image.cols; ++col, pixel += channels) {
            for (int c = 0; c < colorChannels; ++c) {
                pixel[c] = transfer(pixel[c]);
            }
        }
    }
}

// RAII so an early return (parse/transform failure) can't leak an lcms2 handle.
template <class T, void (*Close)(T)>
class LcmsHandle {
public:
    explicit LcmsHandle(T handle) : m_handle(handle) {}
    ~LcmsHandle() {
        if (m_handle) {
            Close(m_handle);
        }
    }
    LcmsHandle(const LcmsHandle&) = delete;
    LcmsHandle& operator=(const LcmsHandle&) = delete;

    T get() const { return m_handle; }
    explicit operator bool() const { return m_handle != nullptr; }

private:
    T m_handle;
};

// cmsCloseProfile actually returns cmsBool, not void, so it can't be used
// as the void(*)(T) template argument directly - wrap it.
void closeProfile(cmsHPROFILE profile) { cmsCloseProfile(profile); }

using ProfileHandle = LcmsHandle<cmsHPROFILE, closeProfile>;
using TransformHandle = LcmsHandle<cmsHTRANSFORM, cmsDeleteTransform>;

} // namespace

void toLinear(cv::Mat& image) { applyTransferInPlace(image, srgbToLinear); }
void toSrgb(cv::Mat& image) { applyTransferInPlace(image, linearToSrgb); }

// cv::cvtColor's RGB<->Lab chain (sRGB decode, 3x3 matrix, cube-root
// nonlinearity, and back) accumulates enough float32 rounding error to
// miss PLAN.md's 1e-3 round-trip budget on some pixels (measured ~3.3e-3
// in CI). Computing it in double precision and rounding back to float32
// only at the boundary - the public contract stays float32 in/out - fixes
// that without deviating from §7.4's "cv::cvtColor on rgb32f" design.
cv::Mat rgbToLab(const cv::Mat& srgbRgb) {
    cv::Mat srgbRgb64, lab64, lab;
    srgbRgb.convertTo(srgbRgb64, CV_64F);
    cv::cvtColor(srgbRgb64, lab64, cv::COLOR_RGB2Lab);
    lab64.convertTo(lab, CV_32F);
    return lab;
}

cv::Mat labToRgb(const cv::Mat& lab) {
    cv::Mat lab64, rgb64, rgb;
    lab.convertTo(lab64, CV_64F);
    cv::cvtColor(lab64, rgb64, cv::COLOR_Lab2RGB);
    rgb64.convertTo(rgb, CV_32F);
    return rgb;
}

Expected<void, ColorError> iccToSrgb(cv::Mat& image, const IccProfile& icc) {
    if (icc.data.empty()) {
        return {};
    }
    CV_Assert(image.depth() == CV_32F && image.channels() == 3);

    ProfileHandle srcProfile(cmsOpenProfileFromMem(icc.data.data(), static_cast<cmsUInt32Number>(icc.data.size())));
    if (!srcProfile) {
        return make_unexpected(ColorError::IccParseFailed);
    }
    ProfileHandle dstProfile(cmsCreate_sRGBProfile());
    if (!dstProfile) {
        return make_unexpected(ColorError::IccTransformFailed);
    }
    TransformHandle transform(cmsCreateTransform(srcProfile.get(), TYPE_RGB_FLT, dstProfile.get(), TYPE_RGB_FLT,
                                                  INTENT_RELATIVE_COLORIMETRIC, 0));
    if (!transform) {
        return make_unexpected(ColorError::IccTransformFailed);
    }

    cv::Mat contiguous = image.isContinuous() ? image : image.clone();
    cmsDoTransform(transform.get(), contiguous.ptr<float>(), contiguous.ptr<float>(),
                    static_cast<cmsUInt32Number>(contiguous.total()));
    if (contiguous.data != image.data) {
        contiguous.copyTo(image);
    }
    return {};
}

} // namespace luma
