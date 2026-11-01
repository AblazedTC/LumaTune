#pragma once

#include <string>

#include "../image/ImageBuffer.h"
#include "../util/Expected.h"

namespace luma {

enum class IoError { FileNotFound, DecodeFailed };

// Decodes an image file into the canonical RGB_F32 [0,1] representation
// (PLAN.md §6.4). EXIF orientation is applied to the pixel data itself,
// matching cv::imread's default auto-rotate behaviour; ImageBuffer's
// exifOrientation records the tag that was read, for export (P1.3) to
// correct re-embedded metadata with - not for callers to apply themselves.
// An embedded ICC profile's raw bytes are stored in ColorSpace.icc and
// left for the ToLinear stage (PLAN.md §7.2) to convert via Transforms::
// iccToSrgb; the decoder itself never calls it.
Expected<ImageBuffer, IoError> decodeImage(const std::string& path);

} // namespace luma
