#include "AutoLevels.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace luma {

namespace {

constexpr int kHistBins = 4096;
constexpr float kLMax = 100.0f;

float histogramPercentile(const cv::Mat& L, float percentile) {
    std::array<std::uint32_t, kHistBins> histogram{};
    for (int row = 0; row < L.rows; ++row) {
        const float* rowPtr = L.ptr<float>(row);
        for (int col = 0; col < L.cols; ++col) {
            const float value = std::clamp(rowPtr[col], 0.0f, kLMax);
            const int bin = static_cast<int>((value / kLMax) * static_cast<float>(kHistBins - 1));
            ++histogram[static_cast<std::size_t>(bin)];
        }
    }

    const double totalPixels = static_cast<double>(L.total());
    const double targetRank = (static_cast<double>(percentile) / 100.0) * (totalPixels - 1.0);
    double cumulative = 0.0;
    for (int bin = 0; bin < kHistBins; ++bin) {
        cumulative += histogram[static_cast<std::size_t>(bin)];
        if (cumulative > targetRank) {
            return (static_cast<float>(bin) + 0.5f) * (kLMax / static_cast<float>(kHistBins));
        }
    }
    return kLMax;
}

} // namespace

LevelsResult autoLevels(const cv::Mat& lab, const LevelsParams& params) {
    CV_Assert(lab.channels() == 3 && lab.depth() == CV_32F);

    std::vector<cv::Mat> channels;
    cv::split(lab, channels);
    const cv::Mat& L = channels[0];

    LevelsDiag diag;
    diag.blackPct = params.blackPct;
    diag.whitePct = params.whitePct;
    double minVal = 0.0;
    double maxVal = 0.0;
    cv::minMaxLoc(L, &minVal, &maxVal);
    diag.inMin = static_cast<float>(minVal);
    diag.inMax = static_cast<float>(maxVal);

    if (params.bypass) {
        diag.blackPt = diag.inMin;
        diag.whitePt = diag.inMax;
        return {lab.clone(), diag};
    }

    float blackPt = histogramPercentile(L, params.blackPct);
    float whitePt = histogramPercentile(L, params.whitePct);
    if (whitePt <= blackPt) {
        whitePt = blackPt + 1.0f; // legacy's guard against a zero-width range
    }
    diag.blackPt = blackPt;
    diag.whitePt = whitePt;

    const double scale = static_cast<double>(kLMax) / (whitePt - blackPt);
    cv::Mat stretched;
    L.convertTo(stretched, CV_32F, scale, -static_cast<double>(blackPt) * scale);
    cv::min(stretched, static_cast<double>(kLMax), stretched);
    cv::max(stretched, 0.0, stretched);
    channels[0] = stretched;

    LevelsResult result;
    cv::merge(channels, result.lab);
    result.diag = diag;
    return result;
}

} // namespace luma
