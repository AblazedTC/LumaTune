# Opt-in sanitizers, wired to the *-asan / *-tsan CMake presets.
# MSVC's /fsanitize=address has different setup requirements (CRT, no UBSan
# equivalent) and isn't wired here yet; win-msvc-asan is a placeholder until
# a task actually needs it.
function(luma_apply_sanitizers target)
  if(MSVC)
    return()
  endif()
  if(LUMA_ENABLE_ASAN)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
  if(LUMA_ENABLE_TSAN)
    target_compile_options(${target} PRIVATE -fsanitize=thread)
    target_link_options(${target} PRIVATE -fsanitize=thread)
  endif()
endfunction()
