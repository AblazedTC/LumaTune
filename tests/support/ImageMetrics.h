#pragma once

#include <cmath>
#include <cstddef>
#include <limits>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace luma::test {

// Peak signal-to-noise ratio between two same-size/-type uint8 images, in
// dB (+inf for identical images). AGENTS.md §9 / PLAN.md T-3.
inline double psnr(const cv::Mat& a, const cv::Mat& b) {
    CV_Assert(a.size() == b.size() && a.type() == b.type());
    cv::Mat diff;
    cv::absdiff(a, b, diff);
    diff.convertTo(diff, CV_64F);
    diff = diff.mul(diff);
    const double mse = cv::sum(diff)[0] /
                        (static_cast<double>(diff.total()) * static_cast<double>(diff.channels()));
    if (mse == 0.0) {
        return std::numeric_limits<double>::infinity();
    }
    return 10.0 * std::log10((255.0 * 255.0) / mse);
}

namespace detail {

constexpr double kPi = 3.14159265358979323846;

// Standard CIEDE2000 formula (Sharma, Wu & Dalal 2005), on true CIE Lab
// values (L in [0,100], a/b roughly [-128,127] - not OpenCV's uint8
// 0-255/128-offset scale).
inline double deltaE2000(double L1, double a1, double b1, double L2, double a2, double b2) {
    const double C1 = std::sqrt(a1 * a1 + b1 * b1);
    const double C2 = std::sqrt(a2 * a2 + b2 * b2);
    const double cbar7 = std::pow((C1 + C2) / 2.0, 7.0);
    const double g = 0.5 * (1.0 - std::sqrt(cbar7 / (cbar7 + std::pow(25.0, 7.0))));
    const double a1p = (1.0 + g) * a1;
    const double a2p = (1.0 + g) * a2;
    const double c1p = std::sqrt(a1p * a1p + b1 * b1);
    const double c2p = std::sqrt(a2p * a2p + b2 * b2);

    const auto wrapDeg = [](double deg) {
        deg = std::fmod(deg, 360.0);
        return deg < 0.0 ? deg + 360.0 : deg;
    };
    const double h1p = (c1p < 1e-9) ? 0.0 : wrapDeg(std::atan2(b1, a1p) * 180.0 / kPi);
    const double h2p = (c2p < 1e-9) ? 0.0 : wrapDeg(std::atan2(b2, a2p) * 180.0 / kPi);

    const double dLp = L2 - L1;
    const double dCp = c2p - c1p;
    const double c1c2 = c1p * c2p;
    double dhp = h2p - h1p;
    if (c1c2 < 1e-9) {
        dhp = 0.0;
    } else if (dhp > 180.0) {
        dhp -= 360.0;
    } else if (dhp < -180.0) {
        dhp += 360.0;
    }
    const double dHp = 2.0 * std::sqrt(c1c2) * std::sin((dhp / 2.0) * kPi / 180.0);

    const double lBarp = (L1 + L2) / 2.0;
    const double cBarp = (c1p + c2p) / 2.0;
    double hBarp;
    if (c1c2 < 1e-9) {
        hBarp = h1p + h2p;
    } else if (std::abs(h1p - h2p) <= 180.0) {
        hBarp = (h1p + h2p) / 2.0;
    } else if (h1p + h2p < 360.0) {
        hBarp = (h1p + h2p + 360.0) / 2.0;
    } else {
        hBarp = (h1p + h2p - 360.0) / 2.0;
    }

    const double t = 1.0 - 0.17 * std::cos((hBarp - 30.0) * kPi / 180.0) +
                      0.24 * std::cos((2.0 * hBarp) * kPi / 180.0) +
                      0.32 * std::cos((3.0 * hBarp + 6.0) * kPi / 180.0) -
                      0.20 * std::cos((4.0 * hBarp - 63.0) * kPi / 180.0);
    const double dTheta = 30.0 * std::exp(-std::pow((hBarp - 275.0) / 25.0, 2.0));
    const double cBarp7 = std::pow(cBarp, 7.0);
    const double rc = 2.0 * std::sqrt(cBarp7 / (cBarp7 + std::pow(25.0, 7.0)));
    const double dLsq = (lBarp - 50.0) * (lBarp - 50.0);
    const double sl = 1.0 + (0.015 * dLsq) / std::sqrt(20.0 + dLsq);
    const double sc = 1.0 + 0.045 * cBarp;
    const double sh = 1.0 + 0.015 * cBarp * t;
    const double rt = -std::sin(2.0 * dTheta * kPi / 180.0) * rc;

    const double termL = dLp / sl;
    const double termC = dCp / sc;
    const double termH = dHp / sh;
    return std::sqrt(termL * termL + termC * termC + termH * termH + rt * termC * termH);
}

} // namespace detail

// Mean (not max - PLAN.md's T-3 row explains why) CIEDE2000 colour
// difference between two same-size uint8 RGB images.
inline double meanDeltaE2000(const cv::Mat& rgbA_u8, const cv::Mat& rgbB_u8) {
    CV_Assert(rgbA_u8.size() == rgbB_u8.size());
    CV_Assert(rgbA_u8.channels() == 3 && rgbB_u8.channels() == 3);

    cv::Mat floatA, floatB, labA, labB;
    rgbA_u8.convertTo(floatA, CV_32F, 1.0 / 255.0);
    rgbB_u8.convertTo(floatB, CV_32F, 1.0 / 255.0);
    cv::cvtColor(floatA, labA, cv::COLOR_RGB2Lab);
    cv::cvtColor(floatB, labB, cv::COLOR_RGB2Lab);

    double total = 0.0;
    for (int row = 0; row < labA.rows; ++row) {
        const auto* rowA = labA.ptr<cv::Vec3f>(row);
        const auto* rowB = labB.ptr<cv::Vec3f>(row);
        for (int col = 0; col < labA.cols; ++col) {
            total += detail::deltaE2000(rowA[col][0], rowA[col][1], rowA[col][2], rowB[col][0],
                                         rowB[col][1], rowB[col][2]);
        }
    }
    return total / (static_cast<double>(labA.rows) * static_cast<double>(labA.cols));
}

} // namespace luma::test
