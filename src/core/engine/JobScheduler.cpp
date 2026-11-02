#include "JobScheduler.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <vector>

namespace luma {

namespace {

unsigned resolveThreadCount(unsigned threadCountOverride) {
    if (threadCountOverride > 0) {
        return threadCountOverride;
    }
    const unsigned hw = std::thread::hardware_concurrency();
    return std::max(2u, (hw > 1) ? hw - 1 : 1u);
}

} // namespace

struct JobScheduler::Impl {
    struct Descriptor {
        DocumentId doc{0};
        JobKind kind{JobKind::Preview};
        std::uint64_t generation{0};
        std::shared_ptr<CancelToken> token{std::make_shared<CancelToken>()};
        JobFn fn;
        std::function<void()> onComplete;
    };

    struct DocState {
        std::uint64_t generation{0};
        std::shared_ptr<Descriptor> previewSlot;
        std::shared_ptr<Descriptor> fullSlot;
    };

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::unordered_map<DocumentId, DocState> m_docs;
    std::deque<std::shared_ptr<Descriptor>> m_previewQueue;
    std::deque<std::shared_ptr<Descriptor>> m_otherQueue; // Full/Export/Batch
    int m_inFlight{0};
    bool m_shuttingDown{false};
    std::vector<std::jthread> m_workers;

    explicit Impl(unsigned threadCountOverride) {
        const unsigned total = resolveThreadCount(threadCountOverride);
        m_workers.emplace_back(
            [this](std::stop_token st) { workerLoop(st, /*previewOnly=*/true); });
        for (unsigned i = 1; i < total; ++i) {
            m_workers.emplace_back(
                [this](std::stop_token st) { workerLoop(st, /*previewOnly=*/false); });
        }
    }

    ~Impl() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shuttingDown = true;
        }
        m_cv.notify_all();
        for (auto& worker : m_workers) {
            worker.request_stop();
        }
        m_cv.notify_all();
        // std::jthread's destructor joins automatically.
    }

    // Cancels and clears whatever currently occupies `slot` (queued or
    // running). Caller holds `m_mutex`.
    static void evictSlot(std::shared_ptr<Descriptor>& slot) {
        if (slot) {
            slot->token->requestCancel();
            slot.reset();
        }
    }

    void submit(DocumentId doc, JobKind kind, JobFn fn, std::function<void()> onComplete) {
        auto descriptor = std::make_shared<Descriptor>();
        descriptor->doc = doc;
        descriptor->kind = kind;
        descriptor->fn = std::move(fn);
        descriptor->onComplete = std::move(onComplete);

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            DocState& state = m_docs[doc];

            if (kind == JobKind::Preview) {
                state.generation += 1;
                descriptor->generation = state.generation;
                evictSlot(state.previewSlot);
                evictSlot(state.fullSlot); // "FULL... cancelled when a newer PREVIEW is submitted"
                state.previewSlot = descriptor;
                m_previewQueue.push_back(descriptor);
            } else if (kind == JobKind::Full) {
                descriptor->generation = state.generation;
                evictSlot(state.fullSlot);
                state.fullSlot = descriptor;
                m_otherQueue.push_back(descriptor);
            } else {
                m_otherQueue.push_back(descriptor); // Export/Batch: no slot, no eviction
            }
        }
        m_cv.notify_all();
    }

    void workerLoop(std::stop_token stoken, bool previewOnly) {
        auto& queue = previewOnly ? m_previewQueue : m_otherQueue;
        while (true) {
            std::shared_ptr<Descriptor> job;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [&] {
                    return stoken.stop_requested() || m_shuttingDown || !queue.empty();
                });
                if (queue.empty()) {
                    if (stoken.stop_requested() || m_shuttingDown) {
                        return;
                    }
                    continue;
                }
                job = queue.front();
                queue.pop_front();

                // Already superseded while queued - never call its fn at
                // all ("removed from queues before they start").
                if (job->token->cancelled()) {
                    continue;
                }
                ++m_inFlight;
            }

            job->fn(*job->token);

            std::function<void()> completion;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                --m_inFlight;
                bool shouldFire = true;
                if (job->kind == JobKind::Preview || job->kind == JobKind::Full) {
                    DocState& state = m_docs[job->doc];
                    auto& slot =
                        (job->kind == JobKind::Preview) ? state.previewSlot : state.fullSlot;
                    shouldFire = (slot == job) && (job->generation == state.generation);
                    if (shouldFire) {
                        slot.reset();
                    }
                }
                if (shouldFire) {
                    completion = job->onComplete;
                }
            }
            m_cv.notify_all(); // wake waitIdle()
            if (completion) {
                completion();
            }
        }
    }

    void waitIdle() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&] {
            return m_previewQueue.empty() && m_otherQueue.empty() && m_inFlight == 0;
        });
    }
};

JobScheduler::JobScheduler(unsigned threadCountOverride)
    : m_impl(std::make_unique<Impl>(threadCountOverride)) {}

JobScheduler::~JobScheduler() = default;

void JobScheduler::submit(DocumentId doc, JobKind kind, JobFn fn,
                           std::function<void()> onComplete) {
    m_impl->submit(doc, kind, std::move(fn), std::move(onComplete));
}

void JobScheduler::waitIdle() { m_impl->waitIdle(); }

} // namespace luma
