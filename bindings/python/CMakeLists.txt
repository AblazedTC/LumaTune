find_package(Python 3.10 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(luma_py src/luma_py.cpp)
target_link_libraries(luma_py PRIVATE luma_core)
luma_set_warnings(luma_py)

# Installed at the wheel root so `import luma_py` finds the extension
# module directly (no wrapper package dir needed - there's no pure-Python
# code here). Only exercised once bindings/python/pyproject.toml (a
# follow-up to this task) actually builds a wheel; harmless no-op for the
# plain `cmake --build` path this CMakeLists.txt is verified against now.
install(TARGETS luma_py LIBRARY DESTINATION .)
