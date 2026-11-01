#include <catch2/catch_test_macros.hpp>

#include <opencv2/core.hpp>

#include "image/BufferPool.h"
#include "stages/classical/Encode.h"

TEST_CASE("Encode/clamps out-of-[0,1] input", "[unit][stages][encode]") {
    luma::ImageBuffer image =
        luma::BufferPool::instance().acquire(2, 2, luma::PixelFormat::RGB_F32, luma::ColorSpace{});
    image.mat().setTo(cv::Scalar(1.5, -0.5, 0.5)); // R/B are linear OETF inputs out of domain on purpose

    luma::ImageBuffer result = luma::encodeStage(image);

    const cv::Vec3f px = result.mat().at<cv::Vec3f>(0, 0);
    CHECK(px[0] <= 1.0f);
    CHECK(px[0] >= 0.0f);
    CHECK(px[1] >= 0.0f);
    CHECK(px[1] <= 1.0f);
}

TEST_CASE("Encode/tags the output as sRGB-encoded", "[unit][stages][encode]") {
    luma::ImageBuffer image =
        luma::BufferPool::instance().acquire(2, 2, luma::PixelFormat::RGB_F32, luma::ColorSpace{});
    image.mat().setTo(cv::Scalar(0.2, 0.2, 0.2));

    luma::ImageBuffer result = luma::encodeStage(image);

    CHECK(result.colorSpace.transfer == luma::Transfer::sRGB);
}

TEST_CASE("Encode/output keeps the input's shape", "[unit][stages][encode]") {
    luma::ImageBuffer image =
        luma::BufferPool::instance().acquire(5, 3, luma::PixelFormat::RGB_F32, luma::ColorSpace{});

    luma::ImageBuffer result = luma::encodeStage(image);

    CHECK(result.width == 5);
    CHECK(result.height == 3);
}
