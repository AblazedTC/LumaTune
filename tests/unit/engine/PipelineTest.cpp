#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>

#include <opencv2/core.hpp>

#include "engine/Pipeline.h"
#include "image/BufferPool.h"

using Catch::Approx;

namespace {
luma::ImageBuffer makeSrgbImage(int size, std::uint64_t seed) {
    luma::ColorSpace srgb{luma::Primaries::sRGB, luma::Transfer::sRGB, std::nullopt};
    luma::ImageBuffer image =
        luma::BufferPool::instance().acquire(size, size, luma::PixelFormat::RGB_F32, srgb);
    cv::RNG rng(seed);
    rng.fill(image.mat(), cv::RNG::UNIFORM, 0.0, 1.0);
    return image;
}

float histogramSum(const luma::Histogram256& hist) {
    return std::accumulate(hist.begin(), hist.end(), 0.0f);
}
} // namespace

TEST_CASE("Pipeline/runs the full chain end-to-end on a synthetic sRGB image",
          "[unit][engine][pipeline]") {
    auto image = makeSrgbImage(16, 11);
    luma::Params params;

    auto result = luma::runPipeline(image, params);
    REQUIRE(result.has_value());

    CHECK(result->image.width == image.width);
    CHECK(result->image.height == image.height);
    CHECK(result->image.format == luma::PixelFormat::RGB_F32);
    CHECK(result->image.colorSpace.transfer == luma::Transfer::sRGB);

    double minVal = 0.0;
    double maxVal = 0.0;
    cv::minMaxLoc(result->image.mat().reshape(1), &minVal, &maxVal);
    CHECK(minVal >= 0.0);
    CHECK(maxVal <= 1.0);
}

TEST_CASE("Pipeline/propagates a stage's ColorError instead of crashing",
          "[unit][engine][pipeline]") {
    luma::ColorSpace adobe{luma::Primaries::AdobeRGB, luma::Transfer::sRGB, std::nullopt};
    luma::ImageBuffer image =
        luma::BufferPool::instance().acquire(8, 8, luma::PixelFormat::RGB_F32, adobe);
    luma::Params params;

    auto result = luma::runPipeline(image, params);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == luma::ColorError::UnsupportedPrimaries);
}

TEST_CASE("Pipeline/diagnostics report one timing per stage and full histograms",
          "[unit][engine][pipeline]") {
    auto image = makeSrgbImage(12, 3);
    luma::Params params;

    auto result = luma::runPipeline(image, params);
    REQUIRE(result.has_value());

    CHECK(result->diag.timings.size() == 7);
    for (const auto& timing : result->diag.timings) {
        CHECK(timing.ms >= 0.0);
        CHECK_FALSE(timing.onGpu);
    }

    const float totalPixels = static_cast<float>(image.width * image.height);
    CHECK(histogramSum(result->diag.lumaBefore) == Approx(totalPixels));
    CHECK(histogramSum(result->diag.lumaAfter) == Approx(totalPixels));
    for (const auto& channelHist : result->diag.rgbAfter) {
        CHECK(histogramSum(channelHist) == Approx(totalPixels));
    }
}

TEST_CASE("Pipeline/bypassing every creative stage round-trips through Lab",
          "[unit][engine][pipeline]") {
    auto image = makeSrgbImage(10, 9);
    const cv::Mat original = image.mat().clone();

    luma::Params params;
    params.wb.bypass = true;
    params.levels.bypass = true;
    params.clahe.bypass = true;
    // curve/saturation already default to bypass=true

    auto result = luma::runPipeline(image, params);
    REQUIRE(result.has_value());

    double maxDiff = 0.0;
    cv::Mat back = result->image.mat();
    for (int row = 0; row < original.rows; ++row) {
        const float* a = original.ptr<float>(row);
        const float* b = back.ptr<float>(row);
        for (int i = 0; i < original.cols * original.channels(); ++i) {
            maxDiff = std::max(maxDiff, static_cast<double>(std::abs(a[i] - b[i])));
        }
    }
    // P1.1's measured Lab round-trip error is ~3.3e-3 (PLAN.md); this also
    // goes through ToLinear/Encode (~1e-5), so budget slightly above 5e-3.
    CHECK(maxDiff < 6e-3);
}
