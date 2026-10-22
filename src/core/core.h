#pragma once

// Placeholder public header for luma_core (PLAN.md P0.4). Real content —
// ImageBuffer, the pipeline engine, stages — lands in Phase 1 (P1.1+).
// This exists only so CI can prove the target configures, builds, links,
// and stays Qt-free on all four platforms before any real code is written.

namespace luma {

// Returns the luma_core ABI/schema version this build was compiled against.
int core_placeholder_version();

} // namespace luma
