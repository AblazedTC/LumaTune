#include "GpuProbe.h"

#include <chrono>

#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/imgproc.hpp>

namespace luma {

namespace {

constexpr int kProbeSize = 1024; // ~1 MP, per PLAN.md §7.6's probe spec

template <class MatT>
double timeCvtColorResizeMs(const cv::Mat& sourceHost) {
    const auto start = std::chrono::steady_clock::now();
    MatT src;
    sourceHost.copyTo(src); // upload, when MatT == cv::UMat
    MatT lab;
    cv::cvtColor(src, lab, cv::COLOR_RGB2Lab);
    MatT resized;
    cv::resize(lab, resized, cv::Size(kProbeSize / 2, kProbeSize / 2), 0, 0, cv::INTER_AREA);
    cv::Mat result;
    resized.copyTo(result); // download, when MatT == cv::UMat
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

bool probeGpuWorthwhile() {
    if (!cv::ocl::haveOpenCL()) {
        return false;
    }

    cv::Mat source(kProbeSize, kProbeSize, CV_32FC3);
    cv::RNG rng(1);
    rng.fill(source, cv::RNG::UNIFORM, 0.0, 1.0);

    try {
        const double cpuMs = timeCvtColorResizeMs<cv::Mat>(source);
        const double gpuMs = timeCvtColorResizeMs<cv::UMat>(source);
        return gpuMs < cpuMs;
    } catch (const cv::Exception&) {
        return false;
    }
}

} // namespace luma
