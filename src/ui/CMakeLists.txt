set(CMAKE_AUTOMOC ON)

qt_add_library(luma_ui STATIC)

qt_add_qml_module(luma_ui
    URI LumaTune.Ui
    VERSION 1.0
    SOURCES
        RecentFilesModel.h
        RecentFilesModel.cpp
)

target_link_libraries(luma_ui PUBLIC Qt6::Core Qt6::Qml)
luma_set_warnings(luma_ui)
