#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include <opencv2/core.hpp>

#include "engine/Params.h"
#include "image/BufferPool.h"
#include "stages/classical/WhiteBalance.h"

using Catch::Approx;

namespace {
luma::ImageBuffer makeSyntheticImage(cv::Scalar lo, cv::Scalar hi, int seed = 7) {
    luma::BufferPool& pool = luma::BufferPool::instance();
    luma::ImageBuffer buffer = pool.acquire(16, 16, luma::PixelFormat::RGB_F32, {});
    cv::Mat mat = buffer.mat();
    cv::RNG rng(seed);
    rng.fill(mat, cv::RNG::UNIFORM, lo, hi);
    return buffer;
}
} // namespace

TEST_CASE("WhiteBalance/GrayWorld chromaticity matches legacy within 1e-4",
          "[unit][stages][whitebalance]") {
    // Non-trivial, unequal per-channel means so the formula is actually
    // exercised (not accidentally trivial at avg_r==avg_g==avg_b).
    luma::ImageBuffer image = makeSyntheticImage({0.1, 0.3, 0.6}, {0.4, 0.5, 0.9});
    const cv::Scalar mean = cv::mean(image.mat());
    const double avgR = mean[0];
    const double avgG = mean[1];
    const double avgB = mean[2];
    const double gray = (avgR + avgG + avgB) / 3.0;
    const double legacyMultR = gray / avgR;
    const double legacyMultG = gray / avgG;
    const double legacyMultB = gray / avgB;

    // strength=1, clamps effectively off, excludes effectively off - see
    // WhiteBalance.h's long comment on why this compares *ratios* against
    // legacy, not raw gain magnitudes (G-normalisation vs legacy's
    // grand-mean normalisation are different, intentional conventions).
    luma::WhiteBalanceParams params;
    params.method = luma::WbMethod::GrayWorld;
    params.strength = 1.0f;
    params.gainMin = 0.0f;
    params.gainMax = 1000.0f;
    params.excludeLow = 0.0f;
    params.excludeHigh = 1.0f;

    auto result = luma::whiteBalance(image, params);
    REQUIRE(result.has_value());

    CHECK(result->diag.gains[1] == Approx(1.0f));
    const double expectedR = legacyMultR / legacyMultG;
    const double expectedB = legacyMultB / legacyMultG;
    CHECK(static_cast<double>(result->diag.gains[0]) == Approx(expectedR).margin(1e-4));
    CHECK(static_cast<double>(result->diag.gains[2]) == Approx(expectedB).margin(1e-4));
}

TEST_CASE("WhiteBalance/bypass returns the input unchanged", "[unit][stages][whitebalance]") {
    luma::ImageBuffer image = makeSyntheticImage({0.1, 0.2, 0.3}, {0.9, 0.8, 0.7});
    luma::WhiteBalanceParams params;
    params.bypass = true;

    auto result = luma::whiteBalance(image, params);
    REQUIRE(result.has_value());

    CHECK(result->diag.gains[0] == 1.0f);
    CHECK(result->diag.gains[1] == 1.0f);
    CHECK(result->diag.gains[2] == 1.0f);
    CHECK(result->diag.confidence == 1.0f);
    const cv::Scalar before = cv::mean(image.mat());
    const cv::Scalar after = cv::mean(result->image.mat());
    CHECK(before[0] == Approx(after[0]));
    CHECK(before[1] == Approx(after[1]));
    CHECK(before[2] == Approx(after[2]));
}

TEST_CASE("WhiteBalance/every method suppresses a dominant red cast",
          "[unit][stages][whitebalance]") {
    // R channel mean is clearly higher than G and B; a correct estimator
    // should respond with gains[R] < gains[G] (G is pinned to 1), for
    // every method - this is the one behaviour all four share by
    // construction (they're all "generalised Gray World").
    luma::ImageBuffer image = makeSyntheticImage({0.6, 0.2, 0.2}, {0.9, 0.4, 0.4});
    const luma::WbMethod methods[] = {luma::WbMethod::GrayWorld, luma::WbMethod::WhitePatch,
                                       luma::WbMethod::ShadesOfGray, luma::WbMethod::GrayEdge};

    for (auto method : methods) {
        luma::WhiteBalanceParams params;
        params.method = method;

        auto result = luma::whiteBalance(image, params);
        REQUIRE(result.has_value());

        CHECK(std::isfinite(result->diag.gains[0]));
        CHECK(std::isfinite(result->diag.gains[1]));
        CHECK(std::isfinite(result->diag.gains[2]));
        CHECK(result->diag.gains[0] < result->diag.gains[1]);
    }
}

TEST_CASE("WhiteBalance/is deterministic", "[unit][stages][whitebalance]") {
    luma::ImageBuffer image = makeSyntheticImage({0.1, 0.3, 0.5}, {0.6, 0.7, 0.8});
    luma::WhiteBalanceParams params;
    params.method = luma::WbMethod::ShadesOfGray;

    auto first = luma::whiteBalance(image, params);
    auto second = luma::whiteBalance(image, params);
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());

    CHECK(first->diag.gains[0] == second->diag.gains[0]);
    CHECK(first->diag.gains[1] == second->diag.gains[1]);
    CHECK(first->diag.gains[2] == second->diag.gains[2]);
    CHECK(cv::norm(first->image.mat(), second->image.mat(), cv::NORM_INF) == 0.0);
}

TEST_CASE("WhiteBalance/output keeps the input's shape and format",
          "[unit][stages][whitebalance]") {
    luma::ImageBuffer image = makeSyntheticImage({0.2, 0.2, 0.2}, {0.8, 0.8, 0.8});
    luma::WhiteBalanceParams params;

    auto result = luma::whiteBalance(image, params);
    REQUIRE(result.has_value());

    CHECK(result->image.width == image.width);
    CHECK(result->image.height == image.height);
    CHECK(result->image.format == image.format);
}

TEST_CASE("WhiteBalance/rejects non-RGB_F32 input instead of reading out of bounds",
          "[unit][stages][whitebalance]") {
    luma::BufferPool& pool = luma::BufferPool::instance();
    luma::ImageBuffer image = pool.acquire(4, 4, luma::PixelFormat::GRAY_F32, {});
    luma::WhiteBalanceParams params;

    auto result = luma::whiteBalance(image, params);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == luma::ColorError::UnsupportedFormat);
}
