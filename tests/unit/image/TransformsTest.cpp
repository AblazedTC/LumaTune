#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

#include <lcms2.h>
#include <opencv2/core.hpp>

#include "image/Transforms.h"

using Catch::Approx;

TEST_CASE("Transforms/srgbToLinear matches the IEC 61966-2-1 formula", "[unit][image][transforms]") {
    REQUIRE(luma::srgbToLinear(0.0f) == 0.0f);
    REQUIRE(luma::srgbToLinear(1.0f) == Approx(1.0f));
    REQUIRE(luma::srgbToLinear(0.02f) == Approx(0.0015479876f)); // low (linear) branch
    REQUIRE(luma::srgbToLinear(0.5f) == Approx(0.21404114f));    // high (power) branch
}

TEST_CASE("Transforms/linearToSrgb matches the IEC 61966-2-1 formula", "[unit][image][transforms]") {
    REQUIRE(luma::linearToSrgb(0.0f) == 0.0f);
    REQUIRE(luma::linearToSrgb(1.0f) == Approx(1.0f));
    REQUIRE(luma::linearToSrgb(0.002f) == Approx(0.02584f));    // low (linear) branch
    REQUIRE(luma::linearToSrgb(0.5f) == Approx(0.73535698f));   // high (power) branch
}

namespace {
double maxAbsDiff(const cv::Mat& a, const cv::Mat& b) {
    REQUIRE(a.size() == b.size());
    REQUIRE(a.channels() == b.channels());
    double maxDiff = 0.0;
    for (int row = 0; row < a.rows; ++row) {
        const float* pa = a.ptr<float>(row);
        const float* pb = b.ptr<float>(row);
        const int n = a.cols * a.channels();
        for (int i = 0; i < n; ++i) {
            maxDiff = std::max(maxDiff, static_cast<double>(std::abs(pa[i] - pb[i])));
        }
    }
    return maxDiff;
}
} // namespace

TEST_CASE("Transforms/toLinear-toSrgb round trip is within 1e-5", "[unit][image][transforms]") {
    cv::Mat image(16, 16, CV_32FC3);
    cv::RNG rng(42);
    rng.fill(image, cv::RNG::UNIFORM, 0.0, 1.0);
    const cv::Mat original = image.clone();

    luma::toLinear(image);
    luma::toSrgb(image);

    REQUIRE(maxAbsDiff(original, image) < 1e-5);
}

TEST_CASE("Transforms/toLinear leaves the alpha channel untouched", "[unit][image][transforms]") {
    cv::Mat image(2, 2, CV_32FC4, cv::Scalar(0.5f, 0.5f, 0.5f, 0.3f));
    luma::toLinear(image);
    for (int row = 0; row < image.rows; ++row) {
        const float* pixel = image.ptr<float>(row);
        for (int col = 0; col < image.cols; ++col) {
            REQUIRE(pixel[col * 4 + 3] == Approx(0.3f));
        }
    }
}

TEST_CASE("Transforms/rgbToLab-labToRgb round trip is within 1e-3", "[unit][image][transforms]") {
    cv::Mat image(16, 16, CV_32FC3);
    cv::RNG rng(7);
    rng.fill(image, cv::RNG::UNIFORM, 0.0, 1.0);

    cv::Mat lab = luma::rgbToLab(image);
    cv::Mat back = luma::labToRgb(lab);

    REQUIRE(maxAbsDiff(image, back) < 1e-3);
}

TEST_CASE("Transforms/iccToSrgb is a no-op on an empty profile", "[unit][image][transforms]") {
    cv::Mat image(2, 2, CV_32FC3, cv::Scalar(0.4f, 0.5f, 0.6f));
    const cv::Mat original = image.clone();

    auto result = luma::iccToSrgb(image, luma::IccProfile{});

    REQUIRE(result.has_value());
    REQUIRE(maxAbsDiff(original, image) == 0.0);
}

namespace {
luma::IccProfile buildSrgbIccProfile() {
    cmsHPROFILE profile = cmsCreate_sRGBProfile();
    REQUIRE(profile != nullptr);
    cmsUInt32Number size = 0;
    cmsSaveProfileToMem(profile, nullptr, &size);
    std::vector<std::byte> bytes(size);
    cmsSaveProfileToMem(profile, bytes.data(), &size);
    cmsCloseProfile(profile);
    return luma::IccProfile{std::move(bytes)};
}
} // namespace

TEST_CASE("Transforms/iccToSrgb through an sRGB profile is ~identity", "[unit][image][transforms]") {
    cv::Mat image(2, 2, CV_32FC3, cv::Scalar(0.4f, 0.5f, 0.6f));
    const cv::Mat original = image.clone();

    auto result = luma::iccToSrgb(image, buildSrgbIccProfile());

    REQUIRE(result.has_value());
    REQUIRE(maxAbsDiff(original, image) < 1e-3);
}
