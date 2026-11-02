#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <tuple>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "engine/Diagnostics.h"
#include "engine/PreviewPolicy.h"
#include "image/BufferPool.h"
#include "image/Transforms.h"
#include "stages/classical/AutoLevels.h"

using Catch::Approx;

namespace {

luma::ImageBuffer makeSolidImage(int width, int height) {
    luma::ImageBuffer image = luma::BufferPool::instance().acquire(
        width, height, luma::PixelFormat::RGB_F32, luma::ColorSpace{});
    image.mat().setTo(cv::Scalar(0.5, 0.5, 0.5));
    return image;
}

// A "gradient + value noise + hard edges" procedural test image (PLAN.md
// T-8: "gradients + Perlin noise + hard edges, seeded"). Value noise - a
// small random grid smoothly upsampled - rather than true Perlin
// (permutation tables, gradient vectors, fade curves): statistically
// close enough for this percentile-closeness test, much simpler to write.
luma::ImageBuffer makeProceduralImage(int width, int height, std::uint64_t seed) {
    cv::Mat lowRes(8, 8, CV_32FC3);
    cv::RNG rng(seed);
    rng.fill(lowRes, cv::RNG::UNIFORM, 0.0, 1.0);
    cv::Mat noise;
    cv::resize(lowRes, noise, cv::Size(width, height), 0, 0, cv::INTER_CUBIC);

    cv::Mat rgb(height, width, CV_32FC3);
    for (int row = 0; row < height; ++row) {
        float* pixel = rgb.ptr<float>(row);
        const float* noiseRow = noise.ptr<float>(row);
        const float gradient = static_cast<float>(row) / static_cast<float>(height - 1);
        for (int col = 0; col < width; ++col) {
            for (int c = 0; c < 3; ++c) {
                const float value = gradient * 0.7f + noiseRow[col * 3 + c] * 0.3f;
                pixel[col * 3 + c] = std::clamp(value, 0.0f, 1.0f);
            }
        }
    }
    // Hard edges: sharp black/white bands, top and bottom.
    const int bandHeight = std::max(1, height / 20);
    rgb(cv::Rect(0, 0, width, bandHeight)).setTo(cv::Scalar(0.0, 0.0, 0.0));
    rgb(cv::Rect(0, height - bandHeight, width, bandHeight)).setTo(cv::Scalar(1.0, 1.0, 1.0));

    luma::ImageBuffer image = luma::BufferPool::instance().acquire(
        width, height, luma::PixelFormat::RGB_F32, luma::ColorSpace{});
    rgb.copyTo(image.mat());
    return image;
}

luma::LevelsDiag levelsDiagOf(const luma::ImageBuffer& image, const luma::LevelsParams& params) {
    const cv::Mat lab = luma::rgbToLab(image.mat());
    return luma::autoLevels(lab, params).diag;
}

} // namespace

TEST_CASE("PreviewPolicy/an oversized image is downscaled to fit both limits",
          "[unit][engine][previewpolicy]") {
    // 4000x3000 = 12 MP, well over kMaxPixels and kMaxLongestEdge.
    auto source = makeSolidImage(4000, 3000);
    const double aspect = static_cast<double>(source.width) / source.height;

    luma::ImageBuffer proxy = luma::buildPreviewProxy(source);

    CHECK(static_cast<long long>(proxy.width) * proxy.height <= luma::PreviewPolicy::kMaxPixels);
    CHECK(std::max(proxy.width, proxy.height) <= luma::PreviewPolicy::kMaxLongestEdge);
    const double proxyAspect = static_cast<double>(proxy.width) / proxy.height;
    CHECK(proxyAspect == Approx(aspect).margin(0.01));
}

TEST_CASE("PreviewPolicy/an already-small image is copied, not referenced",
          "[unit][engine][previewpolicy]") {
    auto source = makeSolidImage(64, 48);
    luma::ImageBuffer proxy = luma::buildPreviewProxy(source);

    CHECK(proxy.width == source.width);
    CHECK(proxy.height == source.height);

    proxy.mat().setTo(cv::Scalar(0.0, 0.0, 0.0));
    CHECK(source.mat().at<cv::Vec3f>(0, 0)[0] == Approx(0.5f)); // source untouched
}

TEST_CASE("PreviewPolicy/proxy percentiles stay within 0.5 L-units of full-res (T-8)",
          "[unit][engine][previewpolicy]") {
    luma::LevelsParams params; // defaults: blackPct=0.5, whitePct=99.5

    // 4000x3000 = 12 MP, 4000x6000 = 24 MP (PLAN.md T-8).
    for (const auto& [width, height, seed] :
         {std::tuple{4000, 3000, std::uint64_t{11}}, std::tuple{4000, 6000, std::uint64_t{22}}}) {
        auto fullRes = makeProceduralImage(width, height, seed);
        auto proxy = luma::buildPreviewProxy(fullRes);

        const luma::LevelsDiag fullDiag = levelsDiagOf(fullRes, params);
        const luma::LevelsDiag proxyDiag = levelsDiagOf(proxy, params);

        CHECK(proxyDiag.blackPt == Approx(fullDiag.blackPt).margin(0.5));
        CHECK(proxyDiag.whitePt == Approx(fullDiag.whitePt).margin(0.5));
    }
}
