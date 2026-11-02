#pragma once

#include "../image/ImageBuffer.h"

namespace luma {

// PLAN.md §7.6: PREVIEW jobs run against a downscaled proxy rather than
// the full-resolution source, built once per document (the caller's
// responsibility to cache - this builder is a pure function, not
// something that memoises on its own). Percentiles computed on the proxy
// are statistically close to full-resolution percentiles - validated by
// T-8 (PreviewPolicyTest.cpp) within 0.5 L-units on multi-megapixel
// procedural inputs.
struct PreviewPolicy {
    static constexpr int kMaxPixels = 1536000; // 1.5 MP
    static constexpr int kMaxLongestEdge = 1600;
};

// Downscales `source` (cv::INTER_AREA, aspect-ratio preserved) just enough
// that neither its total pixel count exceeds PreviewPolicy::kMaxPixels
// nor its longest edge exceeds PreviewPolicy::kMaxLongestEdge. Returns a
// same-size copy (still a fresh BufferPool allocation, not a reference to
// `source`) if it already fits both limits.
ImageBuffer buildPreviewProxy(const ImageBuffer& source);

} // namespace luma
