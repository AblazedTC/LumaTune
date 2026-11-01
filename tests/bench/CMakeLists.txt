find_package(benchmark CONFIG REQUIRED)

add_executable(luma_bench
    StageBench.cpp
)

target_link_libraries(luma_bench PRIVATE luma_core benchmark::benchmark benchmark::benchmark_main)
luma_set_warnings(luma_bench)
