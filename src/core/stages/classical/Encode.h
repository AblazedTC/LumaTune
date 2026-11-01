#pragma once

#include "../../image/ImageBuffer.h"

namespace luma {

// Converts `input` (Linear) to sRGB-encoded, clamped to [0,1] (PLAN.md
// §7.2: "Encode(sRGB OETF, clamp [0,1])" - the only place the sRGB OETF
// is applied in the whole pipeline). Always runs; not user-bypassable
// (no EncodeParams - this is a fixed domain boundary, not a creative
// adjustment, and distinct from io/Encoder.h's file encoder).
ImageBuffer encodeStage(const ImageBuffer& input);

} // namespace luma
