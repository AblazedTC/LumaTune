# Deliberately NOT requesting the Interpreter component: vcpkg's own
# python3 port wraps find_package(Python ...) and unconditionally
# substitutes ITS bundled Python's headers/libs for Development.Module,
# regardless of any Python_EXECUTABLE/Python_ROOT_DIR hint (confirmed
# against vcpkg's actual vcpkg-cmake-wrapper.cmake for python3, not
# assumed). When scikit-build-core also pins Interpreter to the real
# pip-install environment's Python (a different, non-vcpkg version),
# FindPackageHandleStandardArgs's version cross-check between the two
# fails - a real CI failure, not a hypothetical. Dropping Interpreter
# avoids that cross-check entirely; scikit-build-core (or the plain
# cmake --preset ... -DLUMA_BUILD_PY=ON flow) already controls which
# Python actually runs/imports the module, so nanobind_add_module has
# no real need for CMake to separately "find" that same interpreter.
find_package(Python 3.10 COMPONENTS Development.Module REQUIRED)
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(luma_py src/luma_py.cpp)
target_link_libraries(luma_py PRIVATE luma_core)
luma_set_warnings(luma_py)

# Installed at the wheel root so `import luma_py` finds the extension
# module directly (no wrapper package dir needed - there's no pure-Python
# code here).
install(TARGETS luma_py LIBRARY DESTINATION .)
