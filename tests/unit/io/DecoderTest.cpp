#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

#include <opencv2/core.hpp>

#include "io/Decoder.h"
#include "support/TempDir.h"

namespace {
std::string goldenFixture(const std::string& name) {
    return std::string(LUMA_REPO_ROOT) + "/tests/golden/fixtures/" + name;
}
std::string ioFixture(const std::string& name) {
    return std::string(LUMA_REPO_ROOT) + "/tests/unit/fixtures/" + name;
}
} // namespace

TEST_CASE("Decoder/8-bit PNG decodes to RGB_F32", "[unit][io][decoder]") {
    auto result = luma::decodeImage(goldenFixture("neutral.png"));
    REQUIRE(result.has_value());
    CHECK(result->format == luma::PixelFormat::RGB_F32);
    CHECK(result->width == 480);
    CHECK(result->height == 360);
    CHECK_FALSE(result->colorSpace.icc.has_value());
}

TEST_CASE("Decoder/16-bit PNG decodes", "[unit][io][decoder]") {
    auto result = luma::decodeImage(goldenFixture("sixteen_bit.png"));
    REQUIRE(result.has_value());
    CHECK(result->format == luma::PixelFormat::RGB_F32);
    CHECK(result->width == 480);
    CHECK(result->height == 360);
}

TEST_CASE("Decoder/16-bit PNG precision is not downcast to 8-bit", "[unit][io][decoder]") {
    // tests/unit/fixtures/sixteen_bit_precision.png: left half = 10000,
    // right half = 10100 (16-bit). Both round to the *same* 8-bit level
    // (39/255) - if the decoder downcast to 8-bit before reading (A-3's
    // original bug), these would decode identically. Preserved 16-bit
    // precision must show them as measurably different (expected diff
    // 100/65535 ~= 0.0015).
    auto result = luma::decodeImage(ioFixture("sixteen_bit_precision.png"));
    REQUIRE(result.has_value());

    cv::Mat mat = result->mat();
    const float left = mat.at<cv::Vec3f>(mat.rows / 2, 2)[0];
    const float right = mat.at<cv::Vec3f>(mat.rows / 2, mat.cols - 3)[0];
    CHECK(std::abs(right - left) > 0.001f);
}

TEST_CASE("Decoder/WebP decodes", "[unit][io][decoder]") {
    auto result = luma::decodeImage(goldenFixture("sample.webp"));
    REQUIRE(result.has_value());
    CHECK(result->width == 480);
    CHECK(result->height == 360);
}

TEST_CASE("Decoder/16-bit TIFF decodes without downcast", "[unit][io][decoder]") {
    auto result = luma::decodeImage(ioFixture("sixteen_bit.tiff"));
    REQUIRE(result.has_value());
    CHECK(result->format == luma::PixelFormat::RGB_F32);
}

TEST_CASE("Decoder/AdobeRGB-tagged JPEG surfaces the ICC profile", "[unit][io][decoder]") {
    auto result = luma::decodeImage(goldenFixture("adobergb_tagged.jpg"));
    REQUIRE(result.has_value());
    REQUIRE(result->colorSpace.icc.has_value());
    CHECK(result->colorSpace.icc->data.size() > 0);
    CHECK(result->colorSpace.primaries == luma::Primaries::Unknown);
}

TEST_CASE("Decoder/corrupt file returns IoError, doesn't crash", "[unit][io][decoder]") {
    auto result = luma::decodeImage(ioFixture("corrupt.png"));
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == luma::IoError::DecodeFailed);
}

TEST_CASE("Decoder/missing file returns IoError::FileNotFound", "[unit][io][decoder]") {
    luma::test::TempDir dir;
    auto result = luma::decodeImage((dir.path() / "does_not_exist.png").string());
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == luma::IoError::FileNotFound);
}

TEST_CASE("Decoder/all 8 EXIF orientations decode to the same upright layout", "[unit][io][decoder]") {
    // Fixtures: 64x64, quadrants TL=red TR=green BL=blue BR=white *after*
    // orientation correction (tools/gen_io_test_fixtures.py). Sample well
    // inside each quadrant (not at the JPEG-compression-blurred edges).
    for (int tag = 1; tag <= 8; ++tag) {
        auto result = luma::decodeImage(ioFixture("exif_orientation_" + std::to_string(tag) + ".jpg"));
        REQUIRE(result.has_value());
        CHECK(result->width == 64);
        CHECK(result->height == 64);

        cv::Mat mat = result->mat();
        auto closeTo = [](const cv::Vec3f& px, cv::Vec3f expected) {
            constexpr float kTol = 0.15f; // generous: JPEG quality=95 on solid colour
            return std::abs(px[0] - expected[0]) < kTol && std::abs(px[1] - expected[1]) < kTol &&
                   std::abs(px[2] - expected[2]) < kTol;
        };
        CHECK(closeTo(mat.at<cv::Vec3f>(8, 8), {1.f, 0.f, 0.f}));    // top-left: red
        CHECK(closeTo(mat.at<cv::Vec3f>(8, 56), {0.f, 1.f, 0.f}));   // top-right: green
        CHECK(closeTo(mat.at<cv::Vec3f>(56, 8), {0.f, 0.f, 1.f}));   // bottom-left: blue
        CHECK(closeTo(mat.at<cv::Vec3f>(56, 56), {1.f, 1.f, 1.f}));  // bottom-right: white
    }
}
