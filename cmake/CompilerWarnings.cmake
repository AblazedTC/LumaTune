# Warnings-as-errors for every luma_* target (AGENTS.md: "Warnings are errors in CI").
function(luma_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /WX /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      # GCC's -Wmaybe-uninitialized (part of -Wextra) has a well-known
      # false-positive pattern at -O3 through heavily-inlined move/copy
      # chains in templated optional-like types (tl::expected wrapping a
      # struct that holds a std::optional member, here) - not seen on
      # Clang or MSVC. Keep it as a (non-fatal) warning rather than
      # disabling -Wextra wholesale for one flaky check.
      target_compile_options(${target} PRIVATE -Wno-error=maybe-uninitialized)
    endif()
  endif()
endfunction()
