#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "PixelFormat.h"

namespace luma {

// Raw embedded ICC profile bytes, as read from a file. Parsing (lcms2) and
// conversion to sRGB primaries happens in Transforms::iccToSrgb - this
// struct is just the bytes so io/ doesn't need to depend on lcms2.
struct IccProfile {
    std::vector<std::byte> data;
};

struct ColorSpace {
    Primaries primaries{Primaries::Unknown};
    Transfer transfer{Transfer::Unknown};
    std::optional<IccProfile> icc;
};

} // namespace luma
