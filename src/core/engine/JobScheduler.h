#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "CancelToken.h"

namespace luma {

enum class JobKind { Preview, Full, Export, Batch };

// Opaque per-document identifier; the caller (eventually ImageDocument,
// Phase 2) picks it. No meaning to the scheduler beyond equality/hashing.
using DocumentId = std::uint64_t;

// The unit of work a caller submits. Receives a CancelToken to check
// between its own coarse steps - checking it is the job's own
// responsibility (see CancelToken.h).
using JobFn = std::function<void(const CancelToken&)>;

// PLAN.md §7.6. Worker pool: max(2, hw_threads-1) `std::thread`s (not
// `std::jthread` - Apple Clang's libc++ doesn't implement `<stop_token>`
// yet, confirmed by a real CI build failure), one of
// which is reserved for PREVIEW jobs so interactive work never queues
// behind FULL/EXPORT/BATCH.
//
// PREVIEW and FULL are each a latest-only slot per document (depth 1):
// submitting a new one for the same document cancels (via CancelToken)
// and replaces whatever currently occupies that slot, whether it's
// queued or already running. Submitting a PREVIEW also cancels/replaces
// the document's FULL slot ("FULL... cancelled when a newer PREVIEW is
// submitted") and bumps the document's generation; FULL does not bump
// generation itself - PLAN.md's "scheduled 400ms after the last PREVIEW
// completes" debounce is a *timing policy* for the caller to apply
// (e.g. via a UI-layer timer in Phase 2) before calling submit(), not
// something this Qt-free scheduler owns itself.
//
// A PREVIEW/FULL job's `onComplete` only fires if, once the job function
// returns, it's still the current occupant of its slot *and* its
// stamped generation still matches the document's current generation -
// this is the delivery-time backstop for a job that was already running
// (and therefore un-removable from a queue) when it was superseded.
// Queued-but-not-yet-started jobs are dropped before a worker ever calls
// their function at all ("removed from queues before they start").
//
// EXPORT/BATCH take an immutable snapshot semantically (the caller's
// JobFn closes over whatever (source, Params) it needs); the scheduler
// itself just runs them FIFO on the non-preview workers and always
// fires `onComplete` - they're never evicted by PREVIEW/FULL activity.
//
// PreviewPolicy (proxy sizing) and the stage-output cache (PLAN.md §7.5)
// are a separate, still-pending follow-up to this task.
class JobScheduler {
public:
    // threadCountOverride: 0 auto-detects via hardware_concurrency(); a
    // fixed non-zero value gives tests deterministic worker counts.
    explicit JobScheduler(unsigned threadCountOverride = 0);
    ~JobScheduler();

    JobScheduler(const JobScheduler&) = delete;
    JobScheduler& operator=(const JobScheduler&) = delete;

    void submit(DocumentId doc, JobKind kind, JobFn fn, std::function<void()> onComplete = {});

    // Blocks until every queued/running job has drained. Test-only
    // convenience; production code reacts to onComplete instead of
    // polling for idle.
    void waitIdle();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace luma
