#include <catch2/catch_test_macros.hpp>

#include <opencv2/core.hpp>

#include "image/BufferPool.h"
#include "stages/classical/Encode.h"

namespace {
luma::ImageBuffer makeLinearImage(cv::Scalar value) {
    luma::ColorSpace colorSpace{luma::Primaries::sRGB, luma::Transfer::Linear, std::nullopt};
    luma::ImageBuffer image =
        luma::BufferPool::instance().acquire(2, 2, luma::PixelFormat::RGB_F32, colorSpace);
    image.mat().setTo(value);
    return image;
}
} // namespace

TEST_CASE("Encode/clamps out-of-[0,1] input", "[unit][stages][encode]") {
    luma::ImageBuffer image = makeLinearImage({1.5, -0.5, 0.5});

    auto result = luma::encodeStage(image);
    REQUIRE(result.has_value());

    const cv::Vec3f px = result->mat().at<cv::Vec3f>(0, 0);
    CHECK(px[0] <= 1.0f);
    CHECK(px[0] >= 0.0f);
    CHECK(px[1] >= 0.0f);
    CHECK(px[1] <= 1.0f);
}

TEST_CASE("Encode/tags the output as sRGB-encoded", "[unit][stages][encode]") {
    luma::ImageBuffer image = makeLinearImage({0.2, 0.2, 0.2});

    auto result = luma::encodeStage(image);
    REQUIRE(result.has_value());
    CHECK(result->colorSpace.transfer == luma::Transfer::sRGB);
}

TEST_CASE("Encode/output keeps the input's shape", "[unit][stages][encode]") {
    luma::ColorSpace colorSpace{luma::Primaries::sRGB, luma::Transfer::Linear, std::nullopt};
    luma::ImageBuffer image =
        luma::BufferPool::instance().acquire(5, 3, luma::PixelFormat::RGB_F32, colorSpace);

    auto result = luma::encodeStage(image);
    REQUIRE(result.has_value());
    CHECK(result->width == 5);
    CHECK(result->height == 3);
}

TEST_CASE("Encode/rejects non-Linear input instead of double-encoding it",
          "[unit][stages][encode]") {
    luma::ColorSpace colorSpace{luma::Primaries::sRGB, luma::Transfer::sRGB, std::nullopt};
    luma::ImageBuffer image =
        luma::BufferPool::instance().acquire(2, 2, luma::PixelFormat::RGB_F32, colorSpace);

    auto result = luma::encodeStage(image);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == luma::ColorError::WrongTransfer);
}
