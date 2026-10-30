#include "PixelFormat.h"

#include <cassert>

namespace luma {

int channelCount(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGB_F32:
        case PixelFormat::RGB_U16:
        case PixelFormat::RGB_U8:
            return 3;
        case PixelFormat::RGBA_F32:
            return 4;
        case PixelFormat::GRAY_F32:
        case PixelFormat::GRAY_U16:
            return 1;
    }
    assert(false && "unhandled PixelFormat in channelCount");
    return 0;
}

int bytesPerChannel(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGB_F32:
        case PixelFormat::RGBA_F32:
        case PixelFormat::GRAY_F32:
            return 4;
        case PixelFormat::GRAY_U16:
        case PixelFormat::RGB_U16:
            return 2;
        case PixelFormat::RGB_U8:
            return 1;
    }
    assert(false && "unhandled PixelFormat in bytesPerChannel");
    return 0;
}

} // namespace luma
