// Minimal per-stage benchmarks (AGENTS.md §9: "add a benchmark" with each
// stage). The full multi-resolution (1.5/12/24/45 MP) sweep and reference-
// machine baseline recording is PLAN.md task P1.9, once the engine (P1.6)
// exists to run a real pipeline through. Not run by default ctest.
#include <benchmark/benchmark.h>

#include <opencv2/core.hpp>

#include "image/BufferPool.h"
#include "stages/classical/Encode.h"
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
