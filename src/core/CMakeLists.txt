find_package(lcms2 CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(luma_core STATIC
    image/PixelFormat.cpp
    image/ImageBuffer.cpp
    image/BufferPool.cpp
    image/Transforms.cpp
    io/StdDecoder.cpp
    io/Encoder.cpp
    stages/classical/ToLinear.cpp
    stages/classical/Encode.cpp
    stages/classical/WhiteBalance.cpp
    stages/classical/AutoLevels.cpp
    stages/classical/Clahe.cpp
    stages/classical/Curve.cpp
    stages/classical/Saturation.cpp
    engine/Pipeline.cpp
    engine/JobScheduler.cpp
    engine/PreviewPolicy.cpp
    engine/ParamsJson.cpp
    engine/GpuProbe.cpp
)

target_include_directories(luma_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(luma_core PUBLIC cxx_std_20)
# PIC static-library code links fine into a plain executable too (the
# ordinary luma_unit_tests/luma_bench/etc. consumers) - it's only required
# the other direction: luma_py's shared module (bindings/python/) can't
# link a non-PIC static archive into itself. Always-PIC avoids needing two
# separate build configurations of the same library for that one consumer.
set_target_properties(luma_core PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)

target_link_libraries(luma_core
    PUBLIC opencv_core opencv_imgproc opencv_imgcodecs tl::expected Threads::Threads
    PRIVATE lcms2::lcms2 nlohmann_json::nlohmann_json
)

luma_set_warnings(luma_core)
luma_apply_sanitizers(luma_core)
