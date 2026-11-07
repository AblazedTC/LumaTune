#pragma once

namespace luma {

// PLAN.md §7.6/DR-6: runs a ~1 MP cvtColor + resize on cv::UMat, including
// upload and download, timed against the identical operation on cv::Mat.
// Returns true only if the UMat path was faster and didn't throw -
// callers should otherwise leave GPU off for the session ("if slower
// than CPU or it throws, set gpu=Off for the session").
//
// Pure/stateless: doesn't cache its result or touch any session-wide
// flag - there's no Settings/session-lifecycle struct yet for it to live
// in (that's a Phase 2 concern), so running this once at startup and
// persisting the decision is the caller's responsibility, same as
// PreviewPolicy's proxy-caching and JobScheduler's FULL-debounce timing.
bool probeGpuWorthwhile();

} // namespace luma
