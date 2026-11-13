set(CMAKE_AUTOMOC ON)

# T-6 (PLAN.md §11): one executable per QTest class (idiomatic Qt/CMake
# layout - each gets its own QTEST_GUILESS_MAIN-provided main()), both
# guiless since neither ImageDocument nor PipelineController touches the
# QPA platform layer yet (that lands with PreviewItem in a later P2.2
# chunk). ENVIRONMENT below still sets QT_QPA_PLATFORM=offscreen for
# forward consistency with AGENTS.md §4's documented `ui` ctest label.
add_executable(luma_ui_image_document_test ImageDocumentTest.cpp)
target_link_libraries(luma_ui_image_document_test PRIVATE luma_ui Qt6::Test)
target_compile_definitions(luma_ui_image_document_test PRIVATE LUMA_REPO_ROOT="${CMAKE_SOURCE_DIR}")
luma_set_warnings(luma_ui_image_document_test)

add_executable(luma_ui_pipeline_controller_test PipelineControllerTest.cpp)
target_link_libraries(luma_ui_pipeline_controller_test PRIVATE luma_ui Qt6::Test)
target_compile_definitions(luma_ui_pipeline_controller_test PRIVATE LUMA_REPO_ROOT="${CMAKE_SOURCE_DIR}")
luma_set_warnings(luma_ui_pipeline_controller_test)

add_test(NAME ImageDocumentTest COMMAND luma_ui_image_document_test)
add_test(NAME PipelineControllerTest COMMAND luma_ui_pipeline_controller_test)
set_tests_properties(ImageDocumentTest PipelineControllerTest PROPERTIES
    LABELS "ui"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
