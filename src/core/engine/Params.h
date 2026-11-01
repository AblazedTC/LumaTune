#pragma once

namespace luma {

// PLAN.md §7.3. Only what P1.4 needs; LevelsParams/ClaheParams/CurveParams/
// SaturationParams/ExposureParams land with their own stages (P1.5+).
enum class WbMethod { GrayWorld, WhitePatch, ShadesOfGray, GrayEdge };

struct WhiteBalanceParams {
    WbMethod method{WbMethod::GrayWorld};
    float strength{1.f};
    float gainMin{0.5f};
    float gainMax{2.0f};
    float excludeLow{0.02f};
    float excludeHigh{0.98f};
    float sogNorm{6.f}; // Minkowski p-norm for ShadesOfGray / GrayEdge
    bool bypass{false};
};

} // namespace luma
