#pragma once

#include "../../image/ImageBuffer.h"
#include "../../image/Transforms.h"
#include "../../util/Expected.h"

namespace luma {

// Converts `input` (Linear) to sRGB-encoded, clamped to [0,1] (PLAN.md
// §7.2: "Encode(sRGB OETF, clamp [0,1])" - the only place the sRGB OETF
// is applied in the whole pipeline). Always runs; not user-bypassable
// (no EncodeParams - this is a fixed domain boundary, not a creative
// adjustment, and distinct from io/Encoder.h's file encoder).
//
// Returns ColorError::WrongTransfer if `input.colorSpace.transfer` isn't
// Transfer::Linear - applying the OETF to already-encoded input would
// double-encode it silently (AGENTS.md §6: "Stage returns Error::
// WrongTransfer if fed the wrong domain").
Expected<ImageBuffer, ColorError> encodeStage(const ImageBuffer& input);

} // namespace luma
