#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "engine/Params.h"
#include "stages/classical/Clahe.h"

using Catch::Approx;

namespace {

cv::Mat makeFlatLab(int rows, int cols, float L, float a, float b) {
    return cv::Mat(rows, cols, CV_32FC3, cv::Scalar(L, a, b));
}

// Seeded uint8 gradient+noise image, matching legacy's uint8 L domain
// (0-255), sized to an exact multiple of the 8x8 tile grid so neither
// OpenCV's nor ours needs to pad/crop an edge tile - that padding policy
// genuinely differs between the two (see Clahe.h) and isn't what this
// test is checking.
cv::Mat makeUint8Gradient(int size) {
    cv::Mat img(size, size, CV_8UC1);
    for (int row = 0; row < size; ++row) {
        uchar* rowPtr = img.ptr<uchar>(row);
        for (int col = 0; col < size; ++col) {
            rowPtr[col] = static_cast<uchar>((row * 3 + col * 5) % 256);
        }
    }
    return img;
}

} // namespace

TEST_CASE("Clahe/bypass returns the input unchanged", "[unit][stages][clahe]") {
    cv::Mat lab = makeFlatLab(16, 16, 40.0f, 5.0f, -5.0f);
    luma::ClaheParams params;
    params.bypass = true;

    auto result = luma::clahe(lab, params);

    CHECK(cv::norm(lab, result.lab, cv::NORM_INF) == 0.0);
}

TEST_CASE("Clahe/output stays within [0,100] and leaves a/b unchanged",
          "[unit][stages][clahe]") {
    cv::Mat L(32, 32, CV_32F);
    cv::RNG rng(7);
    rng.fill(L, cv::RNG::UNIFORM, 0.0, 100.0);
    cv::Mat a(32, 32, CV_32F, cv::Scalar(3.0f));
    cv::Mat b(32, 32, CV_32F, cv::Scalar(-7.0f));
    cv::Mat lab;
    cv::merge(std::vector<cv::Mat>{L, a, b}, lab);
    std::vector<cv::Mat> before;
    cv::split(lab, before);

    luma::ClaheParams params;
    params.tilesX = 4;
    params.tilesY = 4;

    auto result = luma::clahe(lab, params);

    CHECK(result.lab.rows == lab.rows);
    CHECK(result.lab.cols == lab.cols);
    std::vector<cv::Mat> after;
    cv::split(result.lab, after);
    double minL = 0.0;
    double maxL = 0.0;
    cv::minMaxLoc(after[0], &minL, &maxL);
    CHECK(minL >= 0.0);
    CHECK(maxL <= 100.0);
    CHECK(cv::norm(before[1], after[1], cv::NORM_INF) == 0.0);
    CHECK(cv::norm(before[2], after[2], cv::NORM_INF) == 0.0);
}

TEST_CASE("Clahe/bins=256 matches OpenCV's own 8-bit cv::CLAHE within 1 LSB (DR-9)",
          "[unit][stages][clahe]") {
    constexpr int kSize = 64; // exact multiple of the 8x8 tile grid
    cv::Mat src8u = makeUint8Gradient(kSize);

    cv::Ptr<cv::CLAHE> cvClahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    cv::Mat expected8u;
    cvClahe->apply(src8u, expected8u);

    cv::Mat L;
    src8u.convertTo(L, CV_32F, 100.0 / 255.0);
    cv::Mat zero = cv::Mat::zeros(L.size(), CV_32F);
    cv::Mat lab;
    cv::merge(std::vector<cv::Mat>{L, zero, zero}, lab);

    luma::ClaheParams params;
    params.bins = 256;
    params.clipLimit = 2.0f;
    params.tilesX = 8;
    params.tilesY = 8;

    auto result = luma::clahe(lab, params);

    std::vector<cv::Mat> outChannels;
    cv::split(result.lab, outChannels);
    cv::Mat actual8uFloat;
    outChannels[0].convertTo(actual8uFloat, CV_32F, 255.0 / 100.0);

    cv::Mat expectedFloat;
    expected8u.convertTo(expectedFloat, CV_32F);

    cv::Mat diff;
    cv::absdiff(actual8uFloat, expectedFloat, diff);
    double maxDiff = 0.0;
    cv::minMaxLoc(diff, nullptr, &maxDiff);
    CHECK(maxDiff <= 1.0);
}

TEST_CASE("Clahe/diag reflects the params used", "[unit][stages][clahe]") {
    cv::Mat lab = makeFlatLab(16, 16, 50.0f, 0.0f, 0.0f);
    luma::ClaheParams params;
    params.bins = 512;
    params.clipLimit = 3.5f;
    params.tilesX = 4;
    params.tilesY = 2;

    auto result = luma::clahe(lab, params);

    CHECK(result.diag.bins == 512);
    CHECK(result.diag.clipLimit == Approx(3.5f));
    CHECK(result.diag.tilesX == 4);
    CHECK(result.diag.tilesY == 2);
}
