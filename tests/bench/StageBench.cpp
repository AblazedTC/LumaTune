// Minimal per-stage benchmarks (AGENTS.md §9: "add a benchmark" with each
// stage). The full multi-resolution (1.5/12/24/45 MP) sweep and reference-
// machine baseline recording is PLAN.md task P1.9, once the engine (P1.6)
// exists to run a real pipeline through. Not run by default ctest.
#include <benchmark/benchmark.h>

#include <opencv2/core.hpp>

#include "engine/Pipeline.h"
#include "image/BufferPool.h"
#include "stages/classical/AutoLevels.h"
#include "stages/classical/Clahe.h"
#include "stages/classical/Curve.h"
#include "stages/classical/Encode.h"
#include "stages/classical/Saturation.h"
#include "stages/classical/ToLinear.h"
#include "stages/classical/WhiteBalance.h"

namespace {

constexpr int kBenchSize = 1024; // ~1 MP; full-size sweep lands in P1.9

luma::ImageBuffer makeBenchImage(luma::Transfer transfer) {
    luma::ColorSpace colorSpace{luma::Primaries::sRGB, transfer, std::nullopt};
    luma::BufferPool& pool = luma::BufferPool::instance();
    luma::ImageBuffer image =
        pool.acquire(kBenchSize, kBenchSize, luma::PixelFormat::RGB_F32, colorSpace);
    cv::RNG rng(1);
    rng.fill(image.mat(), cv::RNG::UNIFORM, 0.0, 1.0);
    return image;
}

cv::Mat makeBenchLab() {
    cv::Mat lab(kBenchSize, kBenchSize, CV_32FC3);
    cv::RNG rng(1);
    rng.fill(lab, cv::RNG::UNIFORM, cv::Scalar(0.0, -40.0, -40.0), cv::Scalar(100.0, 40.0, 40.0));
    return lab;
}

} // namespace

static void BM_ToLinear(benchmark::State& state) {
    luma::ImageBuffer image = makeBenchImage(luma::Transfer::sRGB);
    for (auto _ : state) {
        auto result = luma::toLinearStage(image);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ToLinear);

static void BM_Encode(benchmark::State& state) {
    luma::ImageBuffer image = makeBenchImage(luma::Transfer::Linear);
    for (auto _ : state) {
        auto result = luma::encodeStage(image);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Encode);

static void BM_WhiteBalance(benchmark::State& state) {
    luma::ImageBuffer image = makeBenchImage(luma::Transfer::Linear);
    luma::WhiteBalanceParams params;
    for (auto _ : state) {
        auto result = luma::whiteBalance(image, params);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_WhiteBalance);

static void BM_AutoLevels(benchmark::State& state) {
    cv::Mat lab = makeBenchLab();
    luma::LevelsParams params;
    for (auto _ : state) {
        auto result = luma::autoLevels(lab, params);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_AutoLevels);

static void BM_Curve(benchmark::State& state) {
    cv::Mat lab = makeBenchLab();
    luma::CurveParams params;
    params.bypass = false;
    for (auto _ : state) {
        auto result = luma::curve(lab, params);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Curve);

static void BM_Saturation(benchmark::State& state) {
    cv::Mat lab = makeBenchLab();
    luma::SaturationParams params;
    params.bypass = false;
    for (auto _ : state) {
        auto result = luma::saturation(lab, params);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Saturation);

static void BM_Clahe(benchmark::State& state) {
    cv::Mat lab = makeBenchLab();
    luma::ClaheParams params;
    for (auto _ : state) {
        auto result = luma::clahe(lab, params);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Clahe);

static void BM_Pipeline(benchmark::State& state) {
    luma::ImageBuffer image = makeBenchImage(luma::Transfer::sRGB);
    luma::Params params;
    for (auto _ : state) {
        auto result = luma::runPipeline(image, params);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Pipeline);
