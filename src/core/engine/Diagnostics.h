#pragma once

#include <array>
#include <string>
#include <vector>

#include "Params.h"

namespace luma {

// PLAN.md §6.3. Curve/Saturation have no diag struct here, same reasoning
// as ToLinear/Encode (docs/STAGES.md): PLAN.md §6.3 doesn't list one for
// either, and there's no estimation decision to explain - their own
// params already say everything there is to say.
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

// PLAN.md §6.3: "histograms are 256-bin float" on the 0-255 display scale.
using Histogram256 = std::array<float, 256>;

struct StageTiming {
    std::string stage;
    double ms{0.0};
    bool onGpu{false}; // always false until the UMat path lands (P1.10)
};

// PLAN.md §6.3's full aggregate also lists optional<AdaptiveDiag>,
// optional<LearnedDiag>, and jobGeneration - V3/V4 diagnostics and a
// JobScheduler concept (P1.7), none of which exist yet, so they're left
// out here rather than added as unpopulated placeholders.
struct Diagnostics {
    WbDiag wb;
    LevelsDiag levels;
    ClaheDiag clahe;
    Histogram256 lumaBefore{}; // L histogram right after ToLab, pre-AutoLevels
    Histogram256 lumaAfter{};  // L histogram right after Saturation, pre-FromLab
    std::array<Histogram256, 3> rgbAfter{}; // final sRGB output, per channel
    std::vector<StageTiming> timings;
};

} // namespace luma
