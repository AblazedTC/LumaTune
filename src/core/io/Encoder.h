#pragma once

#include <string>

#include "../image/ImageBuffer.h"
#include "../util/Expected.h"
#include "Decoder.h" // IoError

namespace luma {

struct EncodeParams {
    int jpegQuality{90};      // 0-100; JPEG/WebP only
    bool sixteenBitPng{false}; // PNG only; ignored for JPEG/WebP (no 16-bit support)
    bool preserveIcc{true};    // re-embed image.colorSpace.icc if present
};

// Encodes `image` (RGB_F32) to `path`; format is inferred from the path's
// extension (PNG/JPEG/WebP/BMP/TIFF), same convention as cv::imwrite. Values
// outside [0,1] are saturated, not wrapped, by the depth conversion.
//
// EXIF is never re-embedded: by the time an ImageBuffer exists its pixels
// are already displayed-correct (PLAN.md's decode contract, io/Decoder.h),
// so there is nothing to "correct" on write - writing the original
// orientation tag forward would be wrong (PLAN.md §6.4, ADR-0007).
Expected<void, IoError> encodeImage(const std::string& path, const ImageBuffer& image,
                                     const EncodeParams& params = {});

} // namespace luma
