#pragma once

#include <atomic>

namespace luma {

// Cooperative cancellation signal (PLAN.md §7.6): a job function checks
// cancelled() between its own coarse steps. Doesn't forcibly interrupt
// anything - third-party calls inside a job (OpenCV ops, LibRaw's
// dcraw_process, ORT's Run) aren't interruptible, by design; the engine
// bounds the damage architecturally instead (reserved preview worker, at
// most one stale FULL job in flight - see JobScheduler.h).
class CancelToken {
public:
    void requestCancel() noexcept { m_cancelled.store(true, std::memory_order_relaxed); }
    bool cancelled() const noexcept { return m_cancelled.load(std::memory_order_relaxed); }

private:
    std::atomic<bool> m_cancelled{false};
};

} // namespace luma
