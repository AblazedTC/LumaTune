find_package(Catch2 CONFIG REQUIRED)

add_executable(luma_golden_tests
    GoldenTest.cpp
)

target_link_libraries(luma_golden_tests PRIVATE luma_core Catch2::Catch2WithMain)
target_include_directories(luma_golden_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests
                                                      ${CMAKE_SOURCE_DIR}/tests/support)
target_compile_definitions(luma_golden_tests PRIVATE LUMA_REPO_ROOT="${CMAKE_SOURCE_DIR}")
luma_set_warnings(luma_golden_tests)
luma_apply_sanitizers(luma_golden_tests)

include(Catch)
catch_discover_tests(luma_golden_tests PROPERTIES LABELS "golden")
