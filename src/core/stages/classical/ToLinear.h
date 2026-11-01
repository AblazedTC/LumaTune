#pragma once

#include "../../image/ImageBuffer.h"
#include "../../image/Transforms.h"
#include "../../util/Expected.h"

namespace luma {

// Converts `input` to the Linear transfer domain, sRGB primaries, ICC
// cleared (PLAN.md §7.2). Dispatches on input.colorSpace: ICC-tagged
// inputs go through Transforms::iccToSrgb first (then the sRGB EOTF);
// Transfer::sRGB uses the piecewise sRGB EOTF; Transfer::Gamma22 uses a
// pure gamma 2.2 inverse; Transfer::Linear is a copy-through. Always
// runs - not user-bypassable (no ToLinearParams; this is a fixed domain
// boundary, not a creative adjustment).
Expected<ImageBuffer, ColorError> toLinearStage(const ImageBuffer& input);

} // namespace luma
