#pragma once

#include <array>
#include <string>

#include "Params.h"

namespace luma {

// PLAN.md §6.3. Only WbDiag for now; the full Diagnostics aggregate (with
// LevelsDiag, ClaheDiag, histograms, timings, ...) lands in P1.6 when the
// engine actually has stages to aggregate diagnostics from.
struct WbDiag {
    WbMethod used{WbMethod::GrayWorld};
    std::array<float, 3> avgRgb{0.f, 0.f, 0.f}; // the per-method channel statistic, not necessarily a mean
    std::array<float, 3> gains{1.f, 1.f, 1.f};
    float confidence{1.f};
    std::string explanation;
};

} // namespace luma
