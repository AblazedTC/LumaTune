set(CMAKE_AUTOMOC ON)

qt_add_executable(lumatune main.cpp)

qt_add_qml_module(lumatune
    URI LumaTune
    VERSION 1.0
    QML_FILES qml/Main.qml
)

target_link_libraries(lumatune PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)
luma_set_warnings(lumatune)
