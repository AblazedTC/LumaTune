# nanobind's own CMake config hard-requires exactly this invocation
# ("You must invoke 'find_package(Python COMPONENTS Interpreter
# Development REQUIRED)' prior to including nanobind" - confirmed by a
# real CI error, not optional). vcpkg's python3 port wraps find_package
# (Python ...) and, for Development (and Development.Embed), always
# substitutes ITS OWN bundled Python's headers/libs/interpreter
# (NO_DEFAULT_PATH into vcpkg's own tree), ignoring any externally-hinted
# Python_EXECUTABLE - confirmed against vcpkg's actual vcpkg-cmake-
# wrapper.cmake for python3. That's fine and self-consistent *if* the
# interpreter that will actually import the built module afterwards is
# that same vcpkg-bundled version - see the CI workflow's comment next to
# its actions/setup-python step for how that's kept in sync.
find_package(Python 3.10 COMPONENTS Interpreter Development REQUIRED)
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(luma_py src/luma_py.cpp)
target_link_libraries(luma_py PRIVATE luma_core)
luma_set_warnings(luma_py)

# Installed at the wheel root so `import luma_py` finds the extension
# module directly (no wrapper package dir needed - there's no pure-Python
# code here).
install(TARGETS luma_py LIBRARY DESTINATION .)
