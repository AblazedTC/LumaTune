find_package(Catch2 CONFIG REQUIRED)

add_executable(luma_unit_tests
    image/PixelFormatTest.cpp
    image/ImageBufferTest.cpp
    image/BufferPoolTest.cpp
    image/TransformsTest.cpp
)

target_link_libraries(luma_unit_tests PRIVATE luma_core Catch2::Catch2WithMain)
luma_set_warnings(luma_unit_tests)

include(Catch)
catch_discover_tests(luma_unit_tests PROPERTIES LABELS "unit")
