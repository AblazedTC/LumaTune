#include <catch2/catch_test_macros.hpp>

#include <opencv2/core.hpp>

#include "image/BufferPool.h"
#include "image/ImageBuffer.h"

TEST_CASE("ImageBuffer/toCvType maps every PixelFormat", "[unit][image][imagebuffer]") {
    CHECK(luma::toCvType(luma::PixelFormat::RGB_F32) == CV_32FC3);
    CHECK(luma::toCvType(luma::PixelFormat::RGBA_F32) == CV_32FC4);
    CHECK(luma::toCvType(luma::PixelFormat::GRAY_F32) == CV_32FC1);
    CHECK(luma::toCvType(luma::PixelFormat::GRAY_U16) == CV_16UC1);
    CHECK(luma::toCvType(luma::PixelFormat::RGB_U16) == CV_16UC3);
    CHECK(luma::toCvType(luma::PixelFormat::RGB_U8) == CV_8UC3);
}

TEST_CASE("ImageBuffer/host buffer's mat() has the requested shape", "[unit][image][imagebuffer]") {
    luma::ImageBuffer buffer =
        luma::BufferPool::instance().acquire(32, 24, luma::PixelFormat::RGB_F32, luma::ColorSpace{});

    REQUIRE_FALSE(buffer.onDevice());
    cv::Mat mat = buffer.mat();
    CHECK(mat.rows == 24);
    CHECK(mat.cols == 32);
    CHECK(mat.type() == CV_32FC3);

    luma::BufferPool::instance().release(std::move(buffer));
}

TEST_CASE("ImageBuffer/mat() view can be written through", "[unit][image][imagebuffer]") {
    luma::ImageBuffer buffer =
        luma::BufferPool::instance().acquire(4, 4, luma::PixelFormat::GRAY_F32, luma::ColorSpace{});
    cv::Mat mat = buffer.mat();
    mat.setTo(cv::Scalar(0.25f));

    cv::Mat again = buffer.mat();
    CHECK(again.at<float>(0, 0) == 0.25f);
    CHECK(again.at<float>(3, 3) == 0.25f);

    luma::BufferPool::instance().release(std::move(buffer));
}
