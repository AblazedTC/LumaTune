set(CMAKE_AUTOMOC ON)

qt_add_library(luma_ui STATIC)

# New module, no legacy qrc-based resource path to preserve - opt into
# Qt's current default (QTP0001) instead of leaving CI printing an author
# warning on every configure.
qt_policy(SET QTP0001 NEW)

# pragma Singleton in Theme.qml declares intent, but the qmldir entry
# still needs this source property set explicitly - otherwise Main.qml's
# bare `Theme.background` has no singleton instance to resolve against.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# PLAN.md §5: luma_ui owns the whole "LumaTune" QML module (backends +
# the QML that uses them) - lumatune (src/app) is just the launcher
# executable, with no QML module of its own.
qt_add_qml_module(luma_ui
    URI LumaTune
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
    SOURCES
        RecentFilesModel.h
        RecentFilesModel.cpp
        ImageDocument.h
        ImageDocument.cpp
        PipelineController.h
        PipelineController.cpp
        PreviewItem.h
        PreviewItem.cpp
)

target_include_directories(luma_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(luma_ui PUBLIC Qt6::Core Qt6::Qml Qt6::Quick Qt6::QuickControls2 luma_core)
luma_set_warnings(luma_ui)
