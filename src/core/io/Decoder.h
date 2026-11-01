#pragma once

#include <string>

#include "../image/ImageBuffer.h"
#include "../util/Expected.h"

namespace luma {

enum class IoError { FileNotFound, DecodeFailed, EncodeFailed };

// Decodes an image file into the canonical RGB_F32 [0,1] representation
// (PLAN.md §6.4). EXIF orientation is applied to the pixel data itself,
// matching cv::imread's default auto-rotate behaviour; ImageBuffer's
// exifOrientation records the tag that was read (diagnostics/provenance
// only - io/Encoder never writes it back out, since the pixels are
// already displayed-correct by the time an ImageBuffer exists; writing
// the original tag forward would incorrectly re-rotate them).
// An embedded ICC profile's raw bytes are stored in ColorSpace.icc and
// left for the ToLinear stage (PLAN.md §7.2) to convert via Transforms::
// iccToSrgb; the decoder itself never calls it.
Expected<ImageBuffer, IoError> decodeImage(const std::string& path);

} // namespace luma
