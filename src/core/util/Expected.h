#pragma once

#include <tl/expected.hpp>

// AGENTS.md §5 rule 5: core APIs return values, not exceptions. tl::expected
// until all three toolchains ship std::expected (DR-1 - Apple Clang lags).
namespace luma {

template <class T, class E>
using Expected = tl::expected<T, E>;

using tl::make_unexpected;

} // namespace luma
