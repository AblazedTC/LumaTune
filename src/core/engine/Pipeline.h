#pragma once

#include "../image/ImageBuffer.h"
#include "../image/Transforms.h"
#include "../util/Expected.h"
#include "Diagnostics.h"
#include "Params.h"

namespace luma {

struct PipelineResult {
    ImageBuffer image; // RGB_F32, sRGB-encoded, [0,1] - ready for Encoder/display
    Diagnostics diag;
};

// Runs the full classical chain (PLAN.md §7.2):
//   ToLinear -> WhiteBalance -> Encode -> ToLab -> AutoLevels -> Clahe ->
//   Curve -> Saturation -> FromLab
// `input` may be sRGB/Gamma22/ICC-tagged or already Linear; ToLinear
// normalises it. Exposure (RAW-only, V2) and the adaptive/learned stage
// overrides (V3/V4) aren't implemented yet, so they're simply absent from
// the chain rather than wired in as no-ops.
//
// Fails with the first stage's ColorError (only ToLinear/WhiteBalance/
// Encode can fail - AutoLevels/Clahe/Curve/Saturation take a well-formed
// Lab cv::Mat this function itself constructs, so they can't hit their
// CV_Assert guards from here).
Expected<PipelineResult, ColorError> runPipeline(const ImageBuffer& input, const Params& params);

} // namespace luma
