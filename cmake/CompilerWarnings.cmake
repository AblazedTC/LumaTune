# Warnings-as-errors for every luma_* target (AGENTS.md: "Warnings are errors in CI").
function(luma_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /WX /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
  endif()
endfunction()
