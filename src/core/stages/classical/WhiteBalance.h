#pragma once

#include "../../engine/Diagnostics.h"
#include "../../engine/Params.h"
#include "../../image/ImageBuffer.h"

namespace luma {

struct WhiteBalanceResult {
    ImageBuffer image;
    WbDiag diag;
};

// Estimates and applies per-channel gains on `input` (RGB_F32). All four
// methods share one formula shape, generalising GrayWorld's literal
// gains[c] = mean(all)/mean(c) by swapping in a different per-channel
// statistic (PLAN.md §7.3):
//   GrayWorld:    channel mean
//   WhitePatch:   channel 99th percentile
//   ShadesOfGray: channel Minkowski p-norm mean (params.sogNorm)
//   GrayEdge:     Minkowski p-norm mean of the Gaussian-smoothed gradient
//                 magnitude (sigma=1), per channel
//
// Then: exclude pixels with any channel outside [excludeLow, excludeHigh]
// before computing the statistic; clamp gains to [gainMin, gainMax];
// normalise so the G gain is exactly 1; blend toward identity by strength.
//
// NOTE on legacy parity: the shipped pipeline always calls this on LINEAR
// RGB (after ToLinear), which is an intentional behaviour change from V1
// (which ran Gray World directly on sRGB-encoded values, with no gain
// clamping, no exclusion, and no G-normalisation - see legacy/processing/
// white_balance.py). The G-normalisation here means this function's raw
// *gain magnitudes* will never bit-match legacy's (legacy's gains are
// normalised by the grand mean instead, pinning overall brightness rather
// than the G channel) - but the *chromaticity correction* (gains[R]/
// gains[G] and gains[B]/gains[G]) is mathematically identical to legacy's
// (mult_r/mult_g, mult_b/mult_g) when strength=1, clamps disabled
// (gainMin=0, gainMax=huge), and exclusion disabled (excludeLow=0,
// excludeHigh=1), fed sRGB-encoded input directly. That's what the parity
// test (WhiteBalanceTest.cpp) actually checks.
//
// `params.bypass` returns `input` unchanged (ImageBuffer is cheap to
// copy - it shares the underlying pixel storage) with gains (1,1,1).
WhiteBalanceResult whiteBalance(const ImageBuffer& input, const WhiteBalanceParams& params);

} // namespace luma
