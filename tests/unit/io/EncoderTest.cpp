#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

#include <opencv2/core.hpp>

#include "image/BufferPool.h"
#include "io/Decoder.h"
#include "io/Encoder.h"
#include "support/TempDir.h"

namespace {
std::string goldenFixture(const std::string& name) {
    return std::string(LUMA_REPO_ROOT) + "/tests/golden/fixtures/" + name;
}

luma::ImageBuffer makeTestImage() {
    luma::ImageBuffer buffer = luma::BufferPool::instance().acquire(8, 8, luma::PixelFormat::RGB_F32, {});
    cv::Mat mat = buffer.mat();
    for (int row = 0; row < mat.rows; ++row) {
        for (int col = 0; col < mat.cols; ++col) {
            mat.at<cv::Vec3f>(row, col) = cv::Vec3f(0.2f, 0.5f, 0.8f);
        }
    }
    return buffer;
}
} // namespace

TEST_CASE("Encoder/PNG round trip within 8-bit quantisation", "[unit][io][encoder]") {
    luma::test::TempDir dir;
    const std::string path = (dir.path() / "out.png").string();

    auto encodeResult = luma::encodeImage(path, makeTestImage());
    REQUIRE(encodeResult.has_value());

    auto decodeResult = luma::decodeImage(path);
    REQUIRE(decodeResult.has_value());

    cv::Mat mat = decodeResult->mat();
    const cv::Vec3f px = mat.at<cv::Vec3f>(0, 0);
    CHECK(std::abs(px[0] - 0.2f) < 1.0f / 255);
    CHECK(std::abs(px[1] - 0.5f) < 1.0f / 255);
    CHECK(std::abs(px[2] - 0.8f) < 1.0f / 255);
}

TEST_CASE("Encoder/16-bit PNG round trip is tighter than 8-bit", "[unit][io][encoder]") {
    luma::test::TempDir dir;
    const std::string path = (dir.path() / "out16.png").string();

    luma::EncodeParams params;
    params.sixteenBitPng = true;
    auto encodeResult = luma::encodeImage(path, makeTestImage(), params);
    REQUIRE(encodeResult.has_value());

    auto decodeResult = luma::decodeImage(path);
    REQUIRE(decodeResult.has_value());

    cv::Mat mat = decodeResult->mat();
    const cv::Vec3f px = mat.at<cv::Vec3f>(0, 0);
    CHECK(std::abs(px[0] - 0.2f) < 1.0f / 65535);
    CHECK(std::abs(px[1] - 0.5f) < 1.0f / 65535);
    CHECK(std::abs(px[2] - 0.8f) < 1.0f / 65535);
}

TEST_CASE("Encoder/JPEG round trip is approximately correct", "[unit][io][encoder]") {
    luma::test::TempDir dir;
    const std::string path = (dir.path() / "out.jpg").string();

    luma::EncodeParams params;
    params.jpegQuality = 95;
    auto encodeResult = luma::encodeImage(path, makeTestImage(), params);
    REQUIRE(encodeResult.has_value());

    auto decodeResult = luma::decodeImage(path);
    REQUIRE(decodeResult.has_value());

    cv::Mat mat = decodeResult->mat();
    const cv::Vec3f px = mat.at<cv::Vec3f>(4, 4);
    CHECK(std::abs(px[0] - 0.2f) < 0.05f);
    CHECK(std::abs(px[1] - 0.5f) < 0.05f);
    CHECK(std::abs(px[2] - 0.8f) < 0.05f);
}

TEST_CASE("Encoder/WebP round trip is approximately correct", "[unit][io][encoder]") {
    luma::test::TempDir dir;
    const std::string path = (dir.path() / "out.webp").string();

    auto encodeResult = luma::encodeImage(path, makeTestImage());
    REQUIRE(encodeResult.has_value());

    auto decodeResult = luma::decodeImage(path);
    REQUIRE(decodeResult.has_value());
    CHECK(decodeResult->width == 8);
    CHECK(decodeResult->height == 8);
}

TEST_CASE("Encoder/ICC profile round-trips byte-for-byte when preserved", "[unit][io][encoder]") {
    auto source = luma::decodeImage(goldenFixture("adobergb_tagged.jpg"));
    REQUIRE(source.has_value());
    REQUIRE(source->colorSpace.icc.has_value());

    luma::test::TempDir dir;
    const std::string path = (dir.path() / "out.jpg").string();

    auto encodeResult = luma::encodeImage(path, *source); // preserveIcc defaults true
    REQUIRE(encodeResult.has_value());

    auto decodeResult = luma::decodeImage(path);
    REQUIRE(decodeResult.has_value());
    REQUIRE(decodeResult->colorSpace.icc.has_value());
    CHECK(decodeResult->colorSpace.icc->data == source->colorSpace.icc->data);
}

TEST_CASE("Encoder/ICC profile is dropped when preserveIcc is false", "[unit][io][encoder]") {
    auto source = luma::decodeImage(goldenFixture("adobergb_tagged.jpg"));
    REQUIRE(source.has_value());
    REQUIRE(source->colorSpace.icc.has_value());

    luma::test::TempDir dir;
    const std::string path = (dir.path() / "out.jpg").string();

    luma::EncodeParams params;
    params.preserveIcc = false;
    auto encodeResult = luma::encodeImage(path, *source, params);
    REQUIRE(encodeResult.has_value());

    auto decodeResult = luma::decodeImage(path);
    REQUIRE(decodeResult.has_value());
    CHECK_FALSE(decodeResult->colorSpace.icc.has_value());
}

TEST_CASE("Encoder/unwritable path returns IoError, doesn't crash", "[unit][io][encoder]") {
    luma::test::TempDir dir;
    // A directory that doesn't exist, as a "file" path - cannot be opened for writing.
    const std::string path = (dir.path() / "no_such_subdir" / "out.png").string();

    auto encodeResult = luma::encodeImage(path, makeTestImage());
    REQUIRE_FALSE(encodeResult.has_value());
    CHECK(encodeResult.error() == luma::IoError::EncodeFailed);
}
