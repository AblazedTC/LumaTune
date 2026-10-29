cmake_minimum_required(VERSION 3.28)
project(lumatune LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(CompilerWarnings)
include(Sanitizers)

option(LUMA_BUILD_TESTS "Build Catch2 unit/golden/property tests" ON)
option(LUMA_BUILD_BENCH "Build Google Benchmark suite" ON)
option(LUMA_BUILD_PY "Build the luma_py nanobind module" OFF)
option(LUMA_WITH_CUDA "Build ONNX Runtime CUDA execution provider (Linux only)" OFF)
option(LUMA_WITH_COREML "Build ONNX Runtime CoreML execution provider (macOS only)" ON)
option(LUMA_ENABLE_LTO "Enable link-time optimisation in Release builds" ON)
option(LUMA_ENABLE_ASAN "Build with AddressSanitizer + UBSan (non-MSVC)" OFF)
option(LUMA_ENABLE_TSAN "Build with ThreadSanitizer (non-MSVC)" OFF)

find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgcodecs imgproc)

add_subdirectory(src/core)

if(LUMA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/unit)
endif()

# PLAN.md P0.4 acceptance check: prove cv::ocl::haveOpenCL() is reachable
# from this vcpkg baseline on at least one CI runner.
add_executable(opencl_probe tools/opencl_probe.cpp)
target_link_libraries(opencl_probe PRIVATE opencv_core opencv_imgcodecs opencv_imgproc)
luma_set_warnings(opencl_probe)

# luma_ui / lumatune (Qt Quick app) land in Phase 2 (P2.1); luma_py
# (nanobind) and the Google Benchmark suite land through Phase 1 as the
# code they'd exercise is written. Nothing to add_subdirectory() for them
# yet (PLAN.md §14) — wiring empty directories now would just be dead CMake.
