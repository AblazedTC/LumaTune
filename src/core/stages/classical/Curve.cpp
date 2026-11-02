#include "Curve.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace luma {

namespace {

constexpr float kLMax = 100.0f;
constexpr int kNumKnots = 16;
constexpr float kSegment = kLMax / static_cast<float>(kNumKnots - 1);

float evalKnotCurve(const std::array<float, 16>& knots, float value) {
    const float clamped = std::clamp(value, 0.0f, kLMax);
    const float position = clamped / kSegment; // 0..15
    int lowerIdx = static_cast<int>(position);
    if (lowerIdx >= kNumKnots - 1) {
        return knots[static_cast<std::size_t>(kNumKnots - 1)];
    }
    const float frac = position - static_cast<float>(lowerIdx);
    const float lower = knots[static_cast<std::size_t>(lowerIdx)];
    const float upper = knots[static_cast<std::size_t>(lowerIdx + 1)];
    return lower * (1.0f - frac) + upper * frac;
}

void applyKnotCurve(cv::Mat& L, const std::array<float, 16>& knots) {
    for (int row = 0; row < L.rows; ++row) {
        float* rowPtr = L.ptr<float>(row);
        for (int col = 0; col < L.cols; ++col) {
            rowPtr[col] = evalKnotCurve(knots, rowPtr[col]);
        }
    }
}

void clampInPlace(cv::Mat& L) {
    cv::min(L, static_cast<double>(kLMax), L);
    cv::max(L, 0.0, L);
}

} // namespace

cv::Mat curve(const cv::Mat& lab, const CurveParams& params) {
    CV_Assert(lab.channels() == 3 && lab.depth() == CV_32F);
    if (params.bypass) {
        return lab.clone();
    }

    std::vector<cv::Mat> channels;
    cv::split(lab, channels);
    cv::Mat L = channels[0].clone();

    applyKnotCurve(L, params.knots);

    const double contrastScale = 1.0 + static_cast<double>(params.contrast);
    L.convertTo(L, CV_32F, contrastScale, static_cast<double>(kLMax) / 2.0 * (1.0 - contrastScale));
    clampInPlace(L);

    cv::Mat normalized;
    L.convertTo(normalized, CV_32F, 1.0 / static_cast<double>(kLMax));
    cv::Mat powered;
    cv::pow(normalized, 1.0 / static_cast<double>(params.gamma), powered);
    powered.convertTo(L, CV_32F, static_cast<double>(kLMax));
    clampInPlace(L);

    channels[0] = L;
    cv::Mat result;
    cv::merge(channels, result);
    return result;
}

} // namespace luma
