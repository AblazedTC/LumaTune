#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "engine/Params.h"
#include "stages/classical/AutoLevels.h"

using Catch::Approx;

namespace {

// numpy's default ('linear') percentile interpolation, matching legacy/
// processing/levels.py's np.percentile(L, pct) exactly - an independent
// reference to compare the histogram-based implementation against.
float referencePercentile(const cv::Mat& L, float pct) {
    std::vector<float> values(static_cast<std::size_t>(L.total()));
    std::size_t i = 0;
    for (int row = 0; row < L.rows; ++row) {
        const float* rowPtr = L.ptr<float>(row);
        for (int col = 0; col < L.cols; ++col) {
            values[i++] = rowPtr[col];
        }
    }
    std::sort(values.begin(), values.end());
    const double rank = (static_cast<double>(pct) / 100.0) * static_cast<double>(values.size() - 1);
    const auto lower = static_cast<std::size_t>(std::floor(rank));
    const auto upper = static_cast<std::size_t>(std::ceil(rank));
    const double frac = rank - static_cast<double>(lower);
    return static_cast<float>(values[lower] * (1.0 - frac) + values[upper] * frac);
}

cv::Mat makeLabImage(float loL, float hiL, std::uint64_t seed) {
    cv::Mat lab(32, 32, CV_32FC3);
    cv::RNG rng(seed);
    rng.fill(lab, cv::RNG::UNIFORM, cv::Scalar(loL, -10.0, -10.0), cv::Scalar(hiL, 10.0, 10.0));
    return lab;
}

} // namespace

TEST_CASE("AutoLevels/percentiles match a reference within 0.5 L-units",
          "[unit][stages][autolevels]") {
    cv::Mat lab = makeLabImage(10.0f, 90.0f, 11);
    std::vector<cv::Mat> channels;
    cv::split(lab, channels);

    luma::LevelsParams params;
    params.blackPct = 0.5f;
    params.whitePct = 99.5f;

    auto result = luma::autoLevels(lab, params);

    const float refBlack = referencePercentile(channels[0], params.blackPct);
    const float refWhite = referencePercentile(channels[0], params.whitePct);
    CHECK(result.diag.blackPt == Approx(refBlack).margin(0.5));
    CHECK(result.diag.whitePt == Approx(refWhite).margin(0.5));
}

TEST_CASE("AutoLevels/stretches L toward [0,100] and leaves a/b alone",
          "[unit][stages][autolevels]") {
    cv::Mat lab = makeLabImage(20.0f, 80.0f, 3);
    // includes L, but a/b channels [1],[2] are what this test cares about
    const cv::Scalar abBefore = cv::mean(lab);

    luma::LevelsParams params;
    params.blackPct = 1.0f;
    params.whitePct = 99.0f;
    auto result = luma::autoLevels(lab, params);

    double minL = 0.0;
    double maxL = 0.0;
    std::vector<cv::Mat> outChannels;
    cv::split(result.lab, outChannels);
    cv::minMaxLoc(outChannels[0], &minL, &maxL);
    CHECK(minL >= 0.0);
    CHECK(maxL <= 100.0);
    CHECK(maxL > minL + 50.0); // stretched meaningfully, not collapsed

    const cv::Scalar abAfter = cv::mean(result.lab);
    CHECK(abAfter[1] == Approx(abBefore[1]).margin(0.01));
    CHECK(abAfter[2] == Approx(abBefore[2]).margin(0.01));
}

TEST_CASE("AutoLevels/bypass reports input min/max and changes nothing",
          "[unit][stages][autolevels]") {
    cv::Mat lab = makeLabImage(15.0f, 85.0f, 5);
    luma::LevelsParams params;
    params.bypass = true;

    auto result = luma::autoLevels(lab, params);

    CHECK(cv::norm(lab, result.lab, cv::NORM_INF) == 0.0);
    CHECK(result.diag.blackPt == result.diag.inMin);
    CHECK(result.diag.whitePt == result.diag.inMax);
}

TEST_CASE("AutoLevels/a zero-width percentile range doesn't produce NaN/Inf",
          "[unit][stages][autolevels]") {
    cv::Mat lab(8, 8, CV_32FC3, cv::Scalar(50.0, 0.0, 0.0)); // every pixel identical
    luma::LevelsParams params;

    auto result = luma::autoLevels(lab, params);

    std::vector<cv::Mat> outChannels;
    cv::split(result.lab, outChannels);
    for (int row = 0; row < outChannels[0].rows; ++row) {
        const float* rowPtr = outChannels[0].ptr<float>(row);
        for (int col = 0; col < outChannels[0].cols; ++col) {
            CHECK(std::isfinite(rowPtr[col]));
        }
    }
}
