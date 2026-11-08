set(CMAKE_AUTOMOC ON)

qt_add_executable(lumatune main.cpp)

# New module, no legacy qrc-based resource path to preserve - opt into
# Qt's current default (QTP0001) instead of leaving CI printing an author
# warning on every configure.
qt_policy(SET QTP0001 NEW)

# pragma Singleton in Theme.qml declares intent, but the qmldir entry
# still needs this source property set explicitly - otherwise Main.qml's
# bare `Theme.background` has no singleton instance to resolve against.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(lumatune
    URI LumaTune
    VERSION 1.0
    QML_FILES qml/Main.qml qml/Theme.qml
)

target_link_libraries(lumatune PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    luma_ui luma_uiplugin
)
luma_set_warnings(lumatune)
