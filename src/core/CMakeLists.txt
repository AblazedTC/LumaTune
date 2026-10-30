find_package(lcms2 CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)

add_library(luma_core STATIC
    image/PixelFormat.cpp
    image/ImageBuffer.cpp
    image/BufferPool.cpp
    image/Transforms.cpp
)

target_include_directories(luma_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(luma_core PUBLIC cxx_std_20)
set_target_properties(luma_core PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(luma_core PUBLIC opencv_core opencv_imgproc tl::expected PRIVATE lcms2::lcms2)

luma_set_warnings(luma_core)
luma_apply_sanitizers(luma_core)
