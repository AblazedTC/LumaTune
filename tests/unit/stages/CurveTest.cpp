#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

#include <opencv2/core.hpp>

#include "engine/Params.h"
#include "stages/classical/Curve.h"

using Catch::Approx;

namespace {
cv::Mat makeFlatLab(float L, float a, float b) {
    return cv::Mat(4, 4, CV_32FC3, cv::Scalar(L, a, b));
}
} // namespace

TEST_CASE("Curve/bypass returns the input unchanged", "[unit][stages][curve]") {
    cv::Mat lab = makeFlatLab(42.0f, 3.0f, -3.0f);
    luma::CurveParams params; // bypass defaults true

    cv::Mat result = luma::curve(lab, params);

    CHECK(cv::norm(lab, result, cv::NORM_INF) == 0.0);
}

TEST_CASE("Curve/identity knots+contrast+gamma reproduce the input exactly",
          "[unit][stages][curve]") {
    cv::Mat lab = makeFlatLab(37.0f, 2.0f, -5.0f);
    luma::CurveParams params;
    params.bypass = false; // knots/contrast/gamma all at their identity defaults

    cv::Mat result = luma::curve(lab, params);

    const cv::Vec3f px = result.at<cv::Vec3f>(0, 0);
    CHECK(px[0] == Approx(37.0f).margin(1e-4));
    CHECK(px[1] == Approx(2.0f));
    CHECK(px[2] == Approx(-5.0f));
}

TEST_CASE("Curve/gamma matches the documented formula", "[unit][stages][curve]") {
    cv::Mat lab = makeFlatLab(25.0f, 0.0f, 0.0f);
    luma::CurveParams params;
    params.bypass = false;
    params.gamma = 2.0f;

    cv::Mat result = luma::curve(lab, params);

    // L' = 100 * (L/100)^(1/gamma) = 100 * sqrt(0.25) = 50
    CHECK(result.at<cv::Vec3f>(0, 0)[0] == Approx(50.0f).margin(1e-3));
}

TEST_CASE("Curve/contrast matches the documented formula", "[unit][stages][curve]") {
    cv::Mat lab = makeFlatLab(70.0f, 0.0f, 0.0f);
    luma::CurveParams params;
    params.bypass = false;
    params.contrast = 0.5f;

    cv::Mat result = luma::curve(lab, params);

    // L' = (L - 50) * (1 + contrast) + 50 = 20 * 1.5 + 50 = 80
    CHECK(result.at<cv::Vec3f>(0, 0)[0] == Approx(80.0f).margin(1e-3));
}

TEST_CASE("Curve/a raised knot bends the curve at that exact knot position",
          "[unit][stages][curve]") {
    // Knot 8 sits at L = 8 * 100/15 = 53.333...; raising it (keeping
    // monotonicity) should push that exact input value up accordingly,
    // with contrast/gamma left at identity.
    luma::CurveParams params;
    params.bypass = false;
    const float knotL = params.knots[8];
    params.knots[8] = knotL + 10.0f;

    cv::Mat lab = makeFlatLab(knotL, 0.0f, 0.0f);
    cv::Mat result = luma::curve(lab, params);

    CHECK(result.at<cv::Vec3f>(0, 0)[0] == Approx(knotL + 10.0f).margin(1e-3));
}

TEST_CASE("Curve/output stays within [0,100] and keeps the input's shape",
          "[unit][stages][curve]") {
    cv::Mat lab = makeFlatLab(99.0f, 1.0f, 1.0f);
    luma::CurveParams params;
    params.bypass = false;
    params.contrast = 1.0f; // aggressive, to check clamping

    cv::Mat result = luma::curve(lab, params);

    CHECK(result.rows == lab.rows);
    CHECK(result.cols == lab.cols);
    const float value = result.at<cv::Vec3f>(0, 0)[0];
    CHECK(value <= 100.0f);
    CHECK(value >= 0.0f);
}
