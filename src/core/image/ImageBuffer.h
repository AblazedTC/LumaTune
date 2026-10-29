#pragma once

#include <cstddef>
#include <memory>
#include <variant>

#include <opencv2/core.hpp>

#include "ColorSpace.h"
#include "PixelFormat.h"

namespace luma {

// EXIF orientation tag values 1-8 (ISO/IEC 1-8:2019), as read at decode
// time. Stored so encode can round-trip it; luma_core never rotates pixel
// data to "correct" it (io/ applies it once at decode, per PLAN.md §6.4).
enum class Orientation { Normal, Rotate180, Rotate90Cw, Rotate90Ccw, FlipH, FlipV, Transpose, Transverse };

// Host-resident pixel storage: a 64-byte aligned buffer (BufferPool is the
// normal way to get one - this struct just describes what's held).
struct HostStorage {
    std::shared_ptr<void> data;
    std::size_t strideBytes{0};
};

// OpenCL-resident pixel storage (DR-6); unused until the UMat path lands in
// P1.10, but the variant exists now so ImageBuffer's shape is stable.
struct DeviceStorage {
    cv::UMat umat;
};

// Canonical in-memory image representation (PLAN.md §6.1). Immutable once
// handed to the engine; a buffer never mutates its own storage variant.
struct ImageBuffer {
    int width{0};
    int height{0};
    PixelFormat format{PixelFormat::RGB_F32};
    ColorSpace colorSpace;
    std::variant<HostStorage, DeviceStorage> storage;
    Orientation exifOrientation{Orientation::Normal};

    bool onDevice() const { return std::holds_alternative<DeviceStorage>(storage); }

    // Host view of host-resident storage; zero-copy (wraps HostStorage's
    // pointer directly - no refcount tie to it, same as any raw-pointer
    // cv::Mat). Downloads (and owns a copy) when called on device storage.
    // Lifetime: like cv::Mat wrapping any external buffer, the returned Mat
    // is only valid while `this` (or whatever ImageBuffer it was copied
    // from) is still alive - never call .mat() on a temporary ImageBuffer
    // and keep using the result past that expression. The engine also
    // avoids calling this mid GPU-chain (PLAN.md §9) - stages just get what
    // they ask for.
    cv::Mat mat() const;

    // Device view; uploads from host storage if needed.
    cv::UMat umat() const;
};

// OpenCV Mat/UMat type (e.g. CV_32FC3) matching a PixelFormat.
int toCvType(PixelFormat format);

} // namespace luma
