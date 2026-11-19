qt_add_executable(lumatune main.cpp)

# qt_add_executable doesn't set this on its own (confirmed by a real
# CI failure building this on macOS for the first time, P2.8 chunk
# 3/3: without it, lumatune links as a plain Unix binary, not a .app)
# - a real bundle is also what macdeployqt requires (PLAN.md §12's
# macOS row: "macdeployqt LumaTune.app ..."), and what --self-test's
# QCoreApplication path (P2.7) also runs fine from regardless. Ignored
# by CMake on non-Apple platforms.
set_target_properties(lumatune PROPERTIES MACOSX_BUNDLE TRUE)

target_link_libraries(lumatune PRIVATE Qt6::Core Qt6::Gui Qt6::Qml luma_ui luma_uiplugin)
luma_set_warnings(lumatune)
