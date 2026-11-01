#include "WhiteBalance.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "../../image/BufferPool.h"

namespace luma {

namespace {

constexpr float kWhitePatchPercentile = 99.0f;
constexpr float kGrayEdgeSigma = 1.0f;

cv::Mat buildValidMask(const cv::Mat& rgb, float excludeLow, float excludeHigh) {
    std::vector<cv::Mat> channels;
    cv::split(rgb, channels);
    cv::Mat mask;
    cv::inRange(channels[0], excludeLow, excludeHigh, mask);
    for (int c = 1; c < 3; ++c) {
        cv::Mat channelMask;
        cv::inRange(channels[c], excludeLow, excludeHigh, channelMask);
        cv::bitwise_and(mask, channelMask, mask);
    }
    if (cv::countNonZero(mask) == 0) {
        mask.setTo(255); // degenerate: excludes ate the whole image, fall back to no masking
    }
    return mask;
}

std::array<float, 3> channelMean(const cv::Mat& rgb, const cv::Mat& mask) {
    const cv::Scalar m = cv::mean(rgb, mask);
    return {static_cast<float>(m[0]), static_cast<float>(m[1]), static_cast<float>(m[2])};
}

std::array<float, 3> channelPercentile(const cv::Mat& rgb, const cv::Mat& mask, float percentile) {
    std::vector<cv::Mat> channels;
    cv::split(rgb, channels);
    std::array<float, 3> result{};
    std::vector<float> values;
    for (int c = 0; c < 3; ++c) {
        values.clear();
        values.reserve(static_cast<std::size_t>(rgb.rows) * static_cast<std::size_t>(rgb.cols));
        for (int row = 0; row < channels[c].rows; ++row) {
            const float* channelRow = channels[c].ptr<float>(row);
            const unsigned char* maskRow = mask.ptr<unsigned char>(row);
            for (int col = 0; col < channels[c].cols; ++col) {
                if (maskRow[col]) {
                    values.push_back(channelRow[col]);
                }
            }
        }
        if (values.empty()) {
            continue;
        }
        std::sort(values.begin(), values.end());
        const double rank = (percentile / 100.0) * static_cast<double>(values.size() - 1);
        result[c] = values[static_cast<std::size_t>(rank)];
    }
    return result;
}

std::array<float, 3> channelPNormMean(const cv::Mat& rgb, const cv::Mat& mask, float p) {
    std::vector<cv::Mat> channels;
    cv::split(rgb, channels);
    std::array<float, 3> result{};
    for (int c = 0; c < 3; ++c) {
        cv::Mat powered;
        cv::pow(cv::abs(channels[c]), p, powered);
        const cv::Scalar m = cv::mean(powered, mask);
        result[c] = std::pow(static_cast<float>(m[0]), 1.0f / p);
    }
    return result;
}

std::array<float, 3> channelGradientPNormMean(const cv::Mat& rgb, const cv::Mat& mask, float p,
                                               float sigma) {
    cv::Mat blurred;
    cv::GaussianBlur(rgb, blurred, cv::Size(0, 0), sigma);
    std::vector<cv::Mat> channels;
    cv::split(blurred, channels);
    std::array<float, 3> result{};
    for (int c = 0; c < 3; ++c) {
        cv::Mat gx, gy, magnitude, powered;
        cv::Sobel(channels[c], gx, CV_32F, 1, 0);
        cv::Sobel(channels[c], gy, CV_32F, 0, 1);
        cv::magnitude(gx, gy, magnitude);
        cv::pow(magnitude, p, powered);
        const cv::Scalar m = cv::mean(powered, mask);
        result[c] = std::pow(static_cast<float>(m[0]), 1.0f / p);
    }
    return result;
}

std::array<float, 3> gainsFromStatistic(const std::array<float, 3>& stat) {
    const float grand = (stat[0] + stat[1] + stat[2]) / 3.0f;
    std::array<float, 3> gains{};
    for (int c = 0; c < 3; ++c) {
        gains[c] = (stat[c] > 1e-8f) ? (grand / stat[c]) : 1.0f;
    }
    return gains;
}

std::array<float, 3> applyRobustness(std::array<float, 3> gains, const WhiteBalanceParams& params) {
    for (auto& gain : gains) {
        gain = std::clamp(gain, params.gainMin, params.gainMax);
    }
    const float greenGain = gains[1];
    if (greenGain > 1e-8f) {
        for (auto& gain : gains) {
            gain /= greenGain;
        }
    }
    for (auto& gain : gains) {
        gain = params.strength * gain + (1.0f - params.strength);
    }
    return gains;
}

std::string explanationFor(WbMethod method) {
    switch (method) {
        case WbMethod::GrayWorld:
            return "Gray World: assumes the scene averages to neutral gray";
        case WbMethod::WhitePatch:
            return "White Patch: assumes the brightest pixels are neutral";
        case WbMethod::ShadesOfGray:
            return "Shades of Gray: Minkowski-norm generalisation of Gray World";
        case WbMethod::GrayEdge:
            return "Gray Edge: assumes edge gradients average to neutral";
    }
    return "";
}

} // namespace

Expected<WhiteBalanceResult, ColorError> whiteBalance(const ImageBuffer& input,
                                                        const WhiteBalanceParams& params) {
    if (input.format != PixelFormat::RGB_F32) {
        return make_unexpected(ColorError::UnsupportedFormat);
    }

    if (params.bypass) {
        WbDiag diag;
        diag.used = params.method;
        diag.confidence = 1.0f;
        diag.explanation = "White balance bypassed";
        return WhiteBalanceResult{input, diag}; // ImageBuffer copy - shares the pixel storage
    }

    const cv::Mat rgb = input.mat();
    const cv::Mat mask = buildValidMask(rgb, params.excludeLow, params.excludeHigh);

    std::array<float, 3> stat{};
    switch (params.method) {
        case WbMethod::GrayWorld:
            stat = channelMean(rgb, mask);
            break;
        case WbMethod::WhitePatch:
            stat = channelPercentile(rgb, mask, kWhitePatchPercentile);
            break;
        case WbMethod::ShadesOfGray:
            stat = channelPNormMean(rgb, mask, params.sogNorm);
            break;
        case WbMethod::GrayEdge:
            stat = channelGradientPNormMean(rgb, mask, params.sogNorm, kGrayEdgeSigma);
            break;
    }
    const std::array<float, 3> gains = applyRobustness(gainsFromStatistic(stat), params);

    // Confidence: agreement between a GrayWorld and a ShadesOfGray estimate
    // on this same input, regardless of which method is actually selected.
    const std::array<float, 3> gwStat =
        (params.method == WbMethod::GrayWorld) ? stat : channelMean(rgb, mask);
    const std::array<float, 3> sogStat = (params.method == WbMethod::ShadesOfGray)
                                              ? stat
                                              : channelPNormMean(rgb, mask, params.sogNorm);
    const std::array<float, 3> gwGains = applyRobustness(gainsFromStatistic(gwStat), params);
    const std::array<float, 3> sogGains = applyRobustness(gainsFromStatistic(sogStat), params);
    float disagreement = 0.0f;
    for (int c = 0; c < 3; ++c) {
        const float denom = std::max({gwGains[c], sogGains[c], 1e-4f});
        disagreement += std::abs(gwGains[c] - sogGains[c]) / denom;
    }
    const float confidence = 1.0f - std::clamp(disagreement / 3.0f, 0.0f, 1.0f);

    ImageBuffer output =
        BufferPool::instance().acquire(input.width, input.height, input.format, input.colorSpace);
    std::vector<cv::Mat> channels;
    cv::split(rgb, channels);
    for (int c = 0; c < 3; ++c) {
        channels[c] *= static_cast<double>(gains[c]);
    }
    cv::Mat merged;
    cv::merge(channels, merged);
    merged.copyTo(output.mat());
    output.exifOrientation = input.exifOrientation;

    WbDiag diag;
    diag.used = params.method;
    diag.avgRgb = stat;
    diag.gains = gains;
    diag.confidence = confidence;
    diag.explanation = explanationFor(params.method);
    return WhiteBalanceResult{output, diag};
}

} // namespace luma
