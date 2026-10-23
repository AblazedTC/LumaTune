// PLAN.md P0.4: CI must prove OpenCV's OpenCL path is reachable from our
// pinned vcpkg baseline on at least one platform. Not a real feature yet —
// deleted or folded into real diagnostics once Phase 1/§9 lands.
#include <cstdio>

#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>

int main() {
    const bool haveOpenCL = cv::ocl::haveOpenCL();
    std::printf("cv::ocl::haveOpenCL() = %s\n", haveOpenCL ? "true" : "false");
    return 0;
}
