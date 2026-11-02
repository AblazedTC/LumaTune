#pragma once

#include <array>

namespace luma {

// PLAN.md §7.3/§6.2. ClaheParams/ExposureParams land with their own stages.
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

struct LevelsParams {
    float blackPct{0.5f};
    float whitePct{99.5f};
    bool bypass{false}; // on by default - matches V1's always-on auto levels
};

// 16 evenly-spaced knot Y-values (X positions are implicit: i*100/15 for
// i in 0..15) already monotonically non-decreasing - PLAN.md §17.2.2: any
// softplus+cumsum decoding from a model's raw output happens before a
// value lands here, not inside the Curve stage itself. Default is the
// identity ramp (0, 100/15, 200/15, ..., 100).
struct CurveParams {
    std::array<float, 16> knots{0.f,       6.66667f,  13.3333f, 20.f,      26.6667f,  33.3333f,
                                 40.f,      46.6667f,  53.3333f, 60.f,      66.6667f,  73.3333f,
                                 80.f,      86.6667f,  93.3333f, 100.f};
    float contrast{0.f}; // -1..1, pivots around L=50; 0 = identity
    float gamma{1.f};    // >1 brightens, <1 darkens; 1 = identity
    bool bypass{true};   // off by default - new in the rewrite, not a V1 behaviour
};

struct SaturationParams {
    float scale{1.f}; // multiplies Lab a/b about neutral (0); 1 = identity
    bool bypass{true}; // off by default - new in the rewrite, not a V1 behaviour
};

// PLAN.md DR-9: own float CLAHE. clipLimit/tilesX/tilesY default to
// legacy's values (legacy/processing/pipeline.py's ProcessingParams:
// clahe_clip=2.0, clahe_tile=8). bins=256 reproduces OpenCV's/legacy's
// 8-bit semantics for parity; 1024 is the shipping default (banding-free
// on 16-bit export, PLAN.md §6.3).
struct ClaheParams {
    int bins{1024};
    float clipLimit{2.0f};
    int tilesX{8};
    int tilesY{8};
    bool bypass{false}; // on by default - matches V1's always-on CLAHE
};

} // namespace luma
