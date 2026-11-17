# Warnings-as-errors for every luma_* target (AGENTS.md: "Warnings are errors in CI").
function(luma_set_warnings target)
  if(MSVC)
    # /wd4702 (unreachable code): fires from Qt's own headers
    # (qmetatype.h, qvariant.h, qjsengine.h) under MSVC's /W4 on any
    # target that includes them - first hit building luma_ui/lumatune
    # on MSVC at all (P2.8's Windows UI CI job). Not our code; same
    # reasoning as the GCC -Wno-error=maybe-uninitialized exception
    # below (a known false positive from a third-party header, not
    # worth disabling /WX wholesale for).
    target_compile_options(${target} PRIVATE /W4 /permissive- /WX /utf-8 /wd4702)
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
