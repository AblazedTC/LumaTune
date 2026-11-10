qt_add_executable(lumatune main.cpp)

target_link_libraries(lumatune PRIVATE Qt6::Core Qt6::Gui Qt6::Qml luma_ui luma_uiplugin)
luma_set_warnings(lumatune)
