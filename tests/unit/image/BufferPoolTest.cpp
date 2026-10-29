#include <catch2/catch_test_macros.hpp>

#include "image/BufferPool.h"

TEST_CASE("BufferPool/release then acquire reuses the same allocation", "[unit][image][bufferpool]") {
    luma::BufferPool pool;

    luma::ImageBuffer first = pool.acquire(16, 16, luma::PixelFormat::RGB_F32, luma::ColorSpace{});
    const void* firstPtr = std::get<luma::HostStorage>(first.storage).data.get();
    pool.release(std::move(first));

    CHECK(pool.freeBytes() == static_cast<std::size_t>(16 * 16 * 3 * 4));

    luma::ImageBuffer second = pool.acquire(16, 16, luma::PixelFormat::RGB_F32, luma::ColorSpace{});
    const void* secondPtr = std::get<luma::HostStorage>(second.storage).data.get();

    CHECK(firstPtr == secondPtr);
    CHECK(pool.freeBytes() == 0);

    pool.release(std::move(second));
}

TEST_CASE("BufferPool/different shapes don't share a free list", "[unit][image][bufferpool]") {
    luma::BufferPool pool;

    luma::ImageBuffer rgb = pool.acquire(8, 8, luma::PixelFormat::RGB_F32, luma::ColorSpace{});
    pool.release(std::move(rgb));

    luma::ImageBuffer gray = pool.acquire(8, 8, luma::PixelFormat::GRAY_F32, luma::ColorSpace{});
    // A pool entry exists only for RGB_F32 8x8, not GRAY_F32 8x8, so this
    // must be a fresh allocation, and the RGB_F32 entry must still be free.
    CHECK(pool.freeBytes() == static_cast<std::size_t>(8 * 8 * 3 * 4));

    pool.release(std::move(gray));
}
