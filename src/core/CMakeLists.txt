add_library(luma_core STATIC
    core.cpp
)

target_include_directories(luma_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(luma_core PUBLIC cxx_std_20)
set_target_properties(luma_core PROPERTIES CXX_EXTENSIONS OFF)

luma_set_warnings(luma_core)
luma_apply_sanitizers(luma_core)
