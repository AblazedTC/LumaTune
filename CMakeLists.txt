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
# Defaults OFF for now even though src/app ships (repo map, AGENTS.md §2):
# P2.1 chunk 1 is proving the Qt toolchain itself (first-ever Qt install
# in this repo's CI) in its own isolated job, same reasoning as
# LUMA_BUILD_PY's isolated luma-py job in P1.8 - an iteration/failure
# there shouldn't risk the already-stable core build matrix. Flip the
# default once Qt is proven across all 4 platforms (tracked by PLAN.md's
# P2.1 row).
option(LUMA_BUILD_UI "Build luma_ui and the lumatune app (needs Qt 6.11.x)" OFF)

find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgcodecs imgproc)

add_subdirectory(src/core)

if(LUMA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/unit)
    add_subdirectory(tests/golden)
endif()

if(LUMA_BUILD_BENCH)
    add_subdirectory(tests/bench)
endif()

if(LUMA_BUILD_PY)
    add_subdirectory(bindings/python)
endif()

if(LUMA_BUILD_UI)
    include(Qt)
    add_subdirectory(src/app)
endif()

# PLAN.md P0.4 acceptance check: prove cv::ocl::haveOpenCL() is reachable
# from this vcpkg baseline on at least one CI runner.
add_executable(opencl_probe tools/opencl_probe.cpp)
target_link_libraries(opencl_probe PRIVATE opencv_core opencv_imgcodecs opencv_imgproc)
luma_set_warnings(opencl_probe)
