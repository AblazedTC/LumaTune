#pragma once

#include <string>
#include <string_view>

#include "../util/Expected.h"
#include "Diagnostics.h"
#include "Params.h"

namespace luma {

enum class JsonError { ParseFailed, InvalidValue };

// PLAN.md §6.2: "Params serialises to JSON (presets, batch, tests, and
// V4 training labels)." `json` may be partial - any field it omits keeps
// its luma::Params default, so a caller can send just the overrides it
// cares about (e.g. a preset tweaking only `{"wb":{"strength":0.5}}`).
// nlohmann::json is an implementation detail: this header - and every
// caller of it - stays free of that dependency.
Expected<Params, JsonError> paramsFromJson(std::string_view json);

// Round-trips with paramsFromJson (every field present, nothing omitted).
std::string paramsToJson(const Params& params);

// One-way: Diagnostics is an output-only aggregate (histograms, timings,
// per-stage diag structs) - nothing reads it back into a Params/
// Diagnostics value, so there's no diagnosticsFromJson.
std::string diagnosticsToJson(const Diagnostics& diag);

} // namespace luma
