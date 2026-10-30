#include <catch2/catch_test_macros.hpp>

#include "image/PixelFormat.h"

TEST_CASE("PixelFormat/channelCount and bytesPerChannel", "[unit][image][pixelformat]") {
    using luma::PixelFormat;

    CHECK(luma::channelCount(PixelFormat::RGB_F32) == 3);
    CHECK(luma::bytesPerChannel(PixelFormat::RGB_F32) == 4);

    CHECK(luma::channelCount(PixelFormat::RGBA_F32) == 4);
    CHECK(luma::bytesPerChannel(PixelFormat::RGBA_F32) == 4);

    CHECK(luma::channelCount(PixelFormat::GRAY_F32) == 1);
    CHECK(luma::bytesPerChannel(PixelFormat::GRAY_F32) == 4);

    CHECK(luma::channelCount(PixelFormat::GRAY_U16) == 1);
    CHECK(luma::bytesPerChannel(PixelFormat::GRAY_U16) == 2);

    CHECK(luma::channelCount(PixelFormat::RGB_U16) == 3);
    CHECK(luma::bytesPerChannel(PixelFormat::RGB_U16) == 2);

    CHECK(luma::channelCount(PixelFormat::RGB_U8) == 3);
    CHECK(luma::bytesPerChannel(PixelFormat::RGB_U8) == 1);
}
