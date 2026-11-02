#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "engine/JobScheduler.h"

using namespace std::chrono_literals;

namespace {

// Spin-waits (with a generous timeout) for a condition driven by another
// thread - used instead of a fixed sleep to keep the test fast and not
// flaky on a slow CI runner.
template <class Pred>
bool waitFor(Pred pred, std::chrono::milliseconds timeout = 2s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return pred();
}

} // namespace

TEST_CASE("JobScheduler/a submitted job runs and fires onComplete",
          "[unit][engine][jobscheduler]") {
    luma::JobScheduler scheduler(2);
    std::atomic<bool> ran{false};
    std::atomic<bool> completed{false};

    scheduler.submit(1, luma::JobKind::Preview, [&](const luma::CancelToken&) { ran = true; },
                      [&] { completed = true; });

    REQUIRE(waitFor([&] { return completed.load(); }));
    CHECK(ran);
}

TEST_CASE("JobScheduler/cancel+stale-drop+replace-queued: a superseded running job's result is "
          "dropped, a superseded queued job never runs at all, and only the latest survivor "
          "completes",
          "[unit][engine][jobscheduler]") {
    // One preview worker + one other worker, so the single preview slot's
    // occupant is deterministic.
    luma::JobScheduler scheduler(2);

    std::atomic<bool> aStarted{false};
    std::atomic<bool> aCancelledObserved{false};
    std::atomic<bool> aCompleted{false};
    std::mutex gateMutex;
    std::condition_variable gateCv;
    bool releaseA = false;

    // Job A: signals it started, blocks until the test releases it, then
    // records whether it observed its own cancellation request.
    scheduler.submit(
        1, luma::JobKind::Preview,
        [&](const luma::CancelToken& token) {
            aStarted = true;
            std::unique_lock<std::mutex> lock(gateMutex);
            gateCv.wait(lock, [&] { return releaseA; });
            aCancelledObserved = token.cancelled();
        },
        [&] { aCompleted = true; });

    REQUIRE(waitFor([&] { return aStarted.load(); }));

    // Job B: submitted while A is still running, so B sits in the queue -
    // never started before C supersedes it.
    std::atomic<bool> bRan{false};
    std::atomic<bool> bCompleted{false};
    scheduler.submit(1, luma::JobKind::Preview, [&](const luma::CancelToken&) { bRan = true; },
                      [&] { bCompleted = true; });

    // Job C: supersedes B in the (still-queued) preview slot.
    std::atomic<bool> cRan{false};
    std::atomic<bool> cCompleted{false};
    scheduler.submit(1, luma::JobKind::Preview, [&](const luma::CancelToken&) { cRan = true; },
                      [&] { cCompleted = true; });

    {
        std::lock_guard<std::mutex> lock(gateMutex);
        releaseA = true;
    }
    gateCv.notify_all();

    REQUIRE(waitFor([&] { return cCompleted.load(); }));
    scheduler.waitIdle();

    CHECK(aCancelledObserved); // "cancel": A observed the request while running
    CHECK_FALSE(aCompleted);   // A's result was dropped - superseded before it finished

    CHECK_FALSE(bRan);       // "stale-drop"/"replace-queued": B never ran at all
    CHECK_FALSE(bCompleted);

    CHECK(cRan); // only the latest survivor actually completes
    CHECK(cCompleted);
}

TEST_CASE("JobScheduler/a newer PREVIEW cancels a queued FULL for the same document",
          "[unit][engine][jobscheduler]") {
    luma::JobScheduler scheduler(2);

    std::atomic<bool> fullStarted{false};
    std::atomic<bool> fullCompleted{false};
    std::mutex gateMutex;
    std::condition_variable gateCv;
    bool releaseFull = false;

    scheduler.submit(
        1, luma::JobKind::Full,
        [&](const luma::CancelToken&) {
            fullStarted = true;
            std::unique_lock<std::mutex> lock(gateMutex);
            gateCv.wait(lock, [&] { return releaseFull; });
        },
        [&] { fullCompleted = true; });

    REQUIRE(waitFor([&] { return fullStarted.load(); }));

    std::atomic<bool> previewCompleted{false};
    scheduler.submit(1, luma::JobKind::Preview, [&](const luma::CancelToken&) {},
                      [&] { previewCompleted = true; });

    {
        std::lock_guard<std::mutex> lock(gateMutex);
        releaseFull = true;
    }
    gateCv.notify_all();

    REQUIRE(waitFor([&] { return previewCompleted.load(); }));
    scheduler.waitIdle();

    CHECK_FALSE(fullCompleted); // superseded by the newer PREVIEW
}

TEST_CASE("JobScheduler/EXPORT jobs always complete, unaffected by PREVIEW churn",
          "[unit][engine][jobscheduler]") {
    luma::JobScheduler scheduler(2);

    std::atomic<int> exportCompletions{0};
    for (int i = 0; i < 5; ++i) {
        scheduler.submit(1, luma::JobKind::Export, [](const luma::CancelToken&) {},
                          [&] { ++exportCompletions; });
        // Churn the preview slot between export submissions - exports must
        // never be evicted by this.
        scheduler.submit(1, luma::JobKind::Preview, [](const luma::CancelToken&) {});
    }

    REQUIRE(waitFor([&] { return exportCompletions.load() == 5; }));
}

TEST_CASE("JobScheduler/different documents don't interfere with each other's slots",
          "[unit][engine][jobscheduler]") {
    luma::JobScheduler scheduler(2);

    std::atomic<bool> doc1Completed{false};
    std::atomic<bool> doc2Completed{false};
    scheduler.submit(1, luma::JobKind::Preview, [](const luma::CancelToken&) {},
                      [&] { doc1Completed = true; });
    scheduler.submit(2, luma::JobKind::Preview, [](const luma::CancelToken&) {},
                      [&] { doc2Completed = true; });

    REQUIRE(waitFor([&] { return doc1Completed.load() && doc2Completed.load(); }));
}
