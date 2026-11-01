#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "image/BufferPool.h"
#include "io/Decoder.h"
#include "stages/classical/Encode.h"
#include "stages/classical/ToLinear.h"

using Catch::Approx;

namespace {
luma::ImageBuffer makeFlatImage(float value, luma::ColorSpace colorSpace) {
    luma::ImageBuffer buffer =
        luma::BufferPool::instance().acquire(4, 4, luma::PixelFormat::RGB_F32, std::move(colorSpace));
    buffer.mat().setTo(cv::Scalar(value, value, value));
    return buffer;
}
} // namespace

TEST_CASE("ToLinear/sRGB input is linearised and re-tagged", "[unit][stages][tolinear]") {
    auto image = makeFlatImage(0.5f, {luma::Primaries::sRGB, luma::Transfer::sRGB, std::nullopt});
    auto result = luma::toLinearStage(image);
    REQUIRE(result.has_value());

    CHECK(result->colorSpace.transfer == luma::Transfer::Linear);
    CHECK(result->colorSpace.primaries == luma::Primaries::sRGB);
    CHECK_FALSE(result->colorSpace.icc.has_value());
    CHECK(result->mat().at<cv::Vec3f>(0, 0)[0] == Approx(luma::srgbToLinear(0.5f)));
}

TEST_CASE("ToLinear/Linear input is a copy-through", "[unit][stages][tolinear]") {
    auto image = makeFlatImage(0.42f, {luma::Primaries::sRGB, luma::Transfer::Linear, std::nullopt});
    auto result = luma::toLinearStage(image);
    REQUIRE(result.has_value());
    CHECK(result->mat().at<cv::Vec3f>(0, 0)[0] == Approx(0.42f));
}

TEST_CASE("ToLinear/Gamma22 input uses a pure gamma inverse", "[unit][stages][tolinear]") {
    auto image = makeFlatImage(0.5f, {luma::Primaries::sRGB, luma::Transfer::Gamma22, std::nullopt});
    auto result = luma::toLinearStage(image);
    REQUIRE(result.has_value());
    CHECK(result->mat().at<cv::Vec3f>(0, 0)[0] == Approx(std::pow(0.5f, 2.2f)).margin(1e-5));
}

TEST_CASE("ToLinear/invalid ICC data fails cleanly instead of crashing", "[unit][stages][tolinear]") {
    luma::IccProfile garbage;
    garbage.data = std::vector<std::byte>(16, std::byte{0xAB}); // not a real ICC profile
    auto image = makeFlatImage(0.5f, {luma::Primaries::Unknown, luma::Transfer::Unknown, std::move(garbage)});

    auto result = luma::toLinearStage(image);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == luma::ColorError::IccParseFailed);
}

TEST_CASE("ToLinear/a real embedded ICC profile is consumed without error", "[unit][stages][tolinear]") {
    auto decoded = luma::decodeImage(std::string(LUMA_REPO_ROOT) + "/tests/golden/fixtures/adobergb_tagged.jpg");
    REQUIRE(decoded.has_value());
    REQUIRE(decoded->colorSpace.icc.has_value());

    auto result = luma::toLinearStage(*decoded);
    REQUIRE(result.has_value());
    CHECK(result->colorSpace.transfer == luma::Transfer::Linear);
    CHECK_FALSE(result->colorSpace.icc.has_value());
}

TEST_CASE("ToLinear-Encode round trip is within 1e-5 (P1.1's budget)", "[unit][stages][tolinear][encode]") {
    luma::ImageBuffer image =
        luma::BufferPool::instance().acquire(8, 8, luma::PixelFormat::RGB_F32, luma::ColorSpace{});
    cv::RNG rng(5);
    rng.fill(image.mat(), cv::RNG::UNIFORM, 0.0, 1.0);
    image.colorSpace = {luma::Primaries::sRGB, luma::Transfer::sRGB, std::nullopt};
    const cv::Mat original = image.mat().clone();

    auto linear = luma::toLinearStage(image);
    REQUIRE(linear.has_value());
    luma::ImageBuffer encoded = luma::encodeStage(*linear);

    CHECK(encoded.colorSpace.transfer == luma::Transfer::sRGB);
    double maxDiff = 0.0;
    cv::Mat back = encoded.mat();
    for (int row = 0; row < original.rows; ++row) {
        const float* a = original.ptr<float>(row);
        const float* b = back.ptr<float>(row);
        for (int i = 0; i < original.cols * original.channels(); ++i) {
            maxDiff = std::max(maxDiff, static_cast<double>(std::abs(a[i] - b[i])));
        }
    }
    CHECK(maxDiff < 1e-5);
}
