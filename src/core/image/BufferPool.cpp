#include "BufferPool.h"

#include <cstdlib>
#if defined(_MSC_VER)
#include <malloc.h> // _aligned_malloc / _aligned_free
#endif

namespace luma {

namespace {

constexpr std::size_t kAlignment = 64;

std::size_t bytesFor(int width, int height, PixelFormat format) {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
           static_cast<std::size_t>(channelCount(format)) * static_cast<std::size_t>(bytesPerChannel(format));
}

std::shared_ptr<void> allocAligned(std::size_t bytes) {
    std::size_t rounded = ((bytes + kAlignment - 1) / kAlignment) * kAlignment;
#if defined(_MSC_VER)
    void* p = _aligned_malloc(rounded, kAlignment);
    return std::shared_ptr<void>(p, [](void* ptr) { _aligned_free(ptr); });
#else
    void* p = std::aligned_alloc(kAlignment, rounded);
    return std::shared_ptr<void>(p, [](void* ptr) { std::free(ptr); });
#endif
}

} // namespace

std::size_t BufferPool::KeyHash::operator()(const Key& key) const {
    std::size_t h = std::hash<int>{}(key.width);
    h = h * 31 + std::hash<int>{}(key.height);
    h = h * 31 + std::hash<int>{}(static_cast<int>(key.format));
    return h;
}

BufferPool& BufferPool::instance() {
    static BufferPool pool;
    return pool;
}

ImageBuffer BufferPool::acquire(int width, int height, PixelFormat format, ColorSpace colorSpace) {
    const Key key{width, height, format};
    std::shared_ptr<void> data;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_free.find(key);
        if (it != m_free.end() && !it->second.empty()) {
            data = std::move(it->second.back());
            it->second.pop_back();
        }
    }
    if (!data) {
        data = allocAligned(bytesFor(width, height, format));
    }

    ImageBuffer buffer;
    buffer.width = width;
    buffer.height = height;
    buffer.format = format;
    buffer.colorSpace = std::move(colorSpace);
    buffer.storage = HostStorage{std::move(data), 0};
    return buffer;
}

void BufferPool::release(ImageBuffer&& buffer) {
    auto* host = std::get_if<HostStorage>(&buffer.storage);
    if (!host || !host->data) {
        return; // device storage, or already-empty buffer: nothing to pool
    }
    const Key key{buffer.width, buffer.height, buffer.format};
    std::lock_guard<std::mutex> lock(m_mutex);
    m_free[key].push_back(std::move(host->data));
}

std::size_t BufferPool::freeBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::size_t total = 0;
    for (const auto& [key, buffers] : m_free) {
        total += bytesFor(key.width, key.height, key.format) * buffers.size();
    }
    return total;
}

} // namespace luma
