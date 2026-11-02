#include "Pipeline.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "../image/BufferPool.h"
#include "../stages/classical/AutoLevels.h"
#include "../stages/classical/Clahe.h"
#include "../stages/classical/Curve.h"
#include "../stages/classical/Encode.h"
#include "../stages/classical/Saturation.h"
#include "../stages/classical/ToLinear.h"
#include "../stages/classical/WhiteBalance.h"

namespace luma {

namespace {

Histogram256 computeHistogram(const cv::Mat& channel, float rangeMax) {
    Histogram256 hist{};
    for (int row = 0; row < channel.rows; ++row) {
        const float* rowPtr = channel.ptr<float>(row);
        for (int col = 0; col < channel.cols; ++col) {
            const float clamped = std::clamp(rowPtr[col], 0.0f, rangeMax);
            const int bin = static_cast<int>((clamped / rangeMax) * 256.0f);
            ++hist[static_cast<std::size_t>(std::clamp(bin, 0, 255))];
        }
    }
    return hist;
}

template <class Fn>
auto timed(std::vector<StageTiming>& timings, std::string stage, Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    auto result = fn();
    const auto end = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(end - start).count();
    timings.push_back(StageTiming{std::move(stage), ms, false});
    return result;
}

} // namespace

Expected<PipelineResult, ColorError> runPipeline(const ImageBuffer& input, const Params& params) {
    std::vector<StageTiming> timings;

    auto linear = timed(timings, "toLinear", [&] { return toLinearStage(input); });
    if (!linear.has_value()) {
        return make_unexpected(linear.error());
    }

    auto wb = timed(timings, "whiteBalance", [&] { return whiteBalance(*linear, params.wb); });
    if (!wb.has_value()) {
        return make_unexpected(wb.error());
    }

    auto encoded = timed(timings, "encode", [&] { return encodeStage(wb->image); });
    if (!encoded.has_value()) {
        return make_unexpected(encoded.error());
    }

    cv::Mat lab = rgbToLab(encoded->mat());

    std::vector<cv::Mat> labChannelsBefore;
    cv::split(lab, labChannelsBefore);
    const Histogram256 lumaBefore = computeHistogram(labChannelsBefore[0], 100.0f);

    auto levels = timed(timings, "autoLevels", [&] { return autoLevels(lab, params.levels); });
    auto claheResult = timed(timings, "clahe", [&] { return clahe(levels.lab, params.clahe); });
    cv::Mat curved = timed(timings, "curve", [&] { return curve(claheResult.lab, params.curve); });
    cv::Mat saturated =
        timed(timings, "saturation", [&] { return saturation(curved, params.saturation); });

    std::vector<cv::Mat> labChannelsAfter;
    cv::split(saturated, labChannelsAfter);
    const Histogram256 lumaAfter = computeHistogram(labChannelsAfter[0], 100.0f);

    cv::Mat finalRgb = labToRgb(saturated);

    ImageBuffer output = BufferPool::instance().acquire(
        input.width, input.height, PixelFormat::RGB_F32,
        ColorSpace{Primaries::sRGB, Transfer::sRGB, std::nullopt});
    finalRgb.copyTo(output.mat());
    output.exifOrientation = input.exifOrientation;

    std::vector<cv::Mat> rgbChannels;
    cv::split(output.mat(), rgbChannels);
    std::array<Histogram256, 3> rgbAfter{};
    for (std::size_t c = 0; c < 3; ++c) {
        rgbAfter[c] = computeHistogram(rgbChannels[c], 1.0f);
    }

    Diagnostics diag;
    diag.wb = wb->diag;
    diag.levels = levels.diag;
    diag.clahe = claheResult.diag;
    diag.lumaBefore = lumaBefore;
    diag.lumaAfter = lumaAfter;
    diag.rgbAfter = rgbAfter;
    diag.timings = std::move(timings);

    return PipelineResult{output, diag};
}

} // namespace luma
