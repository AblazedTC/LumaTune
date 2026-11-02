find_package(Catch2 CONFIG REQUIRED)

add_executable(luma_unit_tests
    engine/JobSchedulerTest.cpp
    engine/ParamsJsonTest.cpp
    engine/PipelineTest.cpp
    engine/PreviewPolicyTest.cpp
    image/PixelFormatTest.cpp
    image/ImageBufferTest.cpp
    image/BufferPoolTest.cpp
    image/TransformsTest.cpp
    io/DecoderTest.cpp
    io/EncoderTest.cpp
    stages/WhiteBalanceTest.cpp
    stages/ToLinearTest.cpp
    stages/EncodeTest.cpp
    stages/AutoLevelsTest.cpp
    stages/ClaheTest.cpp
    stages/CurveTest.cpp
    stages/SaturationTest.cpp
)

target_link_libraries(luma_unit_tests PRIVATE luma_core Catch2::Catch2WithMain)
target_include_directories(luma_unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_compile_definitions(luma_unit_tests PRIVATE LUMA_REPO_ROOT="${CMAKE_SOURCE_DIR}")
luma_set_warnings(luma_unit_tests)
luma_apply_sanitizers(luma_unit_tests)

include(Catch)
catch_discover_tests(luma_unit_tests PROPERTIES LABELS "unit")
