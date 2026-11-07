#include <catch2/catch_test_macros.hpp>

#include "engine/GpuProbe.h"

// A timing-based heuristic has no single "correct" answer independent of
// the machine running it (PLAN.md's own §11.4: judged on a reference
// machine, never a CI runner) - this just checks it runs to completion
// without throwing and returns a bool either way, on whatever hardware/
// OpenCL backend (real, software/pocl, or none) happens to be present.
TEST_CASE("GpuProbe/runs to completion without throwing", "[unit][engine][gpuprobe]") {
    bool result = false;
    REQUIRE_NOTHROW(result = luma::probeGpuWorthwhile());
    (void)result;
}
