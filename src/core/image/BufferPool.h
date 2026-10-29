#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "ImageBuffer.h"

namespace luma {

// Reuses 64-byte aligned host pixel buffers across pipeline runs, keyed by
// (width, height, format), so repeated preview jobs at the same size don't
// allocate+free a multi-megapixel float buffer on every slider tick
// (PLAN.md §6.1). mutex-guarded: JobScheduler (P1.7) will call this from
// worker threads.
//
// Two safety rules release() enforces:
//  - An allocation only goes back on the free list if this is its one and
//    only owner (use_count() == 1). If a caller copied the ImageBuffer
//    before releasing one copy, pooling the shared allocation would let a
//    future acquire() hand the same memory to a second, unrelated job.
//  - Total free bytes are capped (kMaxFreeBytes); once full, a released
//    allocation just isn't pooled; it frees normally when its last owner
//    goes away. ponytail: reject-when-full, not LRU eviction - add that if
//    profiling ever shows many distinct image shapes churning the cap.
class BufferPool {
public:
    static BufferPool& instance();

    // Returns a host ImageBuffer of the given shape, reusing a pooled
    // allocation if one of the right size is free.
    ImageBuffer acquire(int width, int height, PixelFormat format, ColorSpace colorSpace);

    // Returns the buffer's backing allocation to the pool for reuse. The
    // ImageBuffer itself is consumed; callers drop their copy after this.
    void release(ImageBuffer&& buffer);

    // Total bytes currently held by free (not checked-out) buffers.
    std::size_t freeBytes() const;

private:
    struct Key {
        int width;
        int height;
        PixelFormat format;
        bool operator==(const Key& other) const {
            return width == other.width && height == other.height && format == other.format;
        }
    };
    struct KeyHash {
        std::size_t operator()(const Key& key) const;
    };

    std::size_t freeBytesLocked() const;

    mutable std::mutex m_mutex;
    std::unordered_map<Key, std::vector<std::shared_ptr<void>>, KeyHash> m_free;
};

} // namespace luma
