#pragma once

#include <array>
#include <string>

#include "Params.h"

namespace luma {

// PLAN.md §6.3. The full Diagnostics aggregate (with histograms, timings,
// ...) lands in P1.6 when the engine actually has stages to aggregate
// diagnostics from. Curve/Saturation have no diag struct here, same
// reasoning as ToLinear/Encode (docs/STAGES.md): PLAN.md §6.3 doesn't
// list one for either, and there's no estimation decision to explain -
// their own params already say everything there is to say.
struct WbDiag {
    WbMethod used{WbMethod::GrayWorld};
    std::array<float, 3> avgRgb{0.f, 0.f, 0.f}; // per-method statistic, not always a mean
    std::array<float, 3> gains{1.f, 1.f, 1.f};
    float confidence{1.f};
    std::string explanation;
};

struct LevelsDiag {
    float blackPt{0.f};
    float whitePt{100.f};
    float inMin{0.f};
    float inMax{100.f};
    float blackPct{0.5f};
    float whitePct{99.5f};
};

struct ClaheDiag {
    float clipLimit{2.0f};
    int tilesX{8};
    int tilesY{8};
    int bins{1024};
};

} // namespace luma
