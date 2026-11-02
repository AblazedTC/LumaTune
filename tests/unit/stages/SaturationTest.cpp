#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <opencv2/core.hpp>

#include "engine/Params.h"
#include "stages/classical/Saturation.h"

using Catch::Approx;

namespace {
cv::Mat makeFlatLab(float L, float a, float b) {
    return cv::Mat(4, 4, CV_32FC3, cv::Scalar(L, a, b));
}
} // namespace

TEST_CASE("Saturation/bypass returns the input unchanged", "[unit][stages][saturation]") {
    cv::Mat lab = makeFlatLab(50.0f, 10.0f, -10.0f);
    luma::SaturationParams params; // bypass defaults true

    cv::Mat result = luma::saturation(lab, params);

    CHECK(cv::norm(lab, result, cv::NORM_INF) == 0.0);
}

TEST_CASE("Saturation/scale=1 leaves a/b unchanged", "[unit][stages][saturation]") {
    cv::Mat lab = makeFlatLab(60.0f, 8.0f, -4.0f);
    luma::SaturationParams params;
    params.bypass = false;
    params.scale = 1.0f;

    cv::Mat result = luma::saturation(lab, params);

    const cv::Vec3f px = result.at<cv::Vec3f>(0, 0);
    CHECK(px[0] == Approx(60.0f));
    CHECK(px[1] == Approx(8.0f));
    CHECK(px[2] == Approx(-4.0f));
}

TEST_CASE("Saturation/scales a and b about neutral and leaves L alone",
          "[unit][stages][saturation]") {
    cv::Mat lab = makeFlatLab(55.0f, 10.0f, -6.0f);
    luma::SaturationParams params;
    params.bypass = false;
    params.scale = 2.0f;

    cv::Mat result = luma::saturation(lab, params);

    const cv::Vec3f px = result.at<cv::Vec3f>(0, 0);
    CHECK(px[0] == Approx(55.0f));
    CHECK(px[1] == Approx(20.0f));
    CHECK(px[2] == Approx(-12.0f));
}

TEST_CASE("Saturation/scale=0 desaturates to neutral a/b", "[unit][stages][saturation]") {
    cv::Mat lab = makeFlatLab(45.0f, 15.0f, -9.0f);
    luma::SaturationParams params;
    params.bypass = false;
    params.scale = 0.0f;

    cv::Mat result = luma::saturation(lab, params);

    const cv::Vec3f px = result.at<cv::Vec3f>(0, 0);
    CHECK(px[0] == Approx(45.0f));
    CHECK(px[1] == Approx(0.0f));
    CHECK(px[2] == Approx(0.0f));
}
