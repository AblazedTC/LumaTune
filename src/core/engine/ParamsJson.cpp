#include "ParamsJson.h"

#include <array>
#include <cstdint>

#include <nlohmann/json.hpp>

namespace luma {

namespace {

using Json = nlohmann::json;

const char* wbMethodToString(WbMethod method) {
    switch (method) {
        case WbMethod::GrayWorld:
            return "GrayWorld";
        case WbMethod::WhitePatch:
            return "WhitePatch";
        case WbMethod::ShadesOfGray:
            return "ShadesOfGray";
        case WbMethod::GrayEdge:
            return "GrayEdge";
    }
    return "GrayWorld";
}

WbMethod wbMethodFromString(const std::string& text) {
    if (text == "WhitePatch") {
        return WbMethod::WhitePatch;
    }
    if (text == "ShadesOfGray") {
        return WbMethod::ShadesOfGray;
    }
    if (text == "GrayEdge") {
        return WbMethod::GrayEdge;
    }
    return WbMethod::GrayWorld;
}

template <class T, std::size_t N>
Json arrayToJson(const std::array<T, N>& values) {
    Json array = Json::array();
    for (const T& value : values) {
        array.push_back(value);
    }
    return array;
}

template <class T, std::size_t N>
void arrayFromJson(const Json& j, std::array<T, N>& out) {
    if (!j.is_array() || j.size() != N) {
        return; // leave the caller's default untouched
    }
    for (std::size_t i = 0; i < N; ++i) {
        out[i] = j.at(i).get<T>();
    }
}

void mergeWhiteBalance(const Json& j, WhiteBalanceParams& p) {
    if (j.contains("method") && j["method"].is_string()) {
        p.method = wbMethodFromString(j["method"].get<std::string>());
    }
    p.strength = j.value("strength", p.strength);
    p.gainMin = j.value("gainMin", p.gainMin);
    p.gainMax = j.value("gainMax", p.gainMax);
    p.excludeLow = j.value("excludeLow", p.excludeLow);
    p.excludeHigh = j.value("excludeHigh", p.excludeHigh);
    p.sogNorm = j.value("sogNorm", p.sogNorm);
    p.bypass = j.value("bypass", p.bypass);
}

Json toJson(const WhiteBalanceParams& p) {
    return Json{{"method", wbMethodToString(p.method)},
                {"strength", p.strength},
                {"gainMin", p.gainMin},
                {"gainMax", p.gainMax},
                {"excludeLow", p.excludeLow},
                {"excludeHigh", p.excludeHigh},
                {"sogNorm", p.sogNorm},
                {"bypass", p.bypass}};
}

void mergeLevels(const Json& j, LevelsParams& p) {
    p.blackPct = j.value("blackPct", p.blackPct);
    p.whitePct = j.value("whitePct", p.whitePct);
    p.bypass = j.value("bypass", p.bypass);
}

Json toJson(const LevelsParams& p) {
    return Json{{"blackPct", p.blackPct}, {"whitePct", p.whitePct}, {"bypass", p.bypass}};
}

void mergeClahe(const Json& j, ClaheParams& p) {
    p.bins = j.value("bins", p.bins);
    p.clipLimit = j.value("clipLimit", p.clipLimit);
    p.tilesX = j.value("tilesX", p.tilesX);
    p.tilesY = j.value("tilesY", p.tilesY);
    p.bypass = j.value("bypass", p.bypass);
}

Json toJson(const ClaheParams& p) {
    return Json{{"bins", p.bins},
                {"clipLimit", p.clipLimit},
                {"tilesX", p.tilesX},
                {"tilesY", p.tilesY},
                {"bypass", p.bypass}};
}

void mergeCurve(const Json& j, CurveParams& p) {
    if (j.contains("knots")) {
        arrayFromJson(j["knots"], p.knots);
    }
    p.contrast = j.value("contrast", p.contrast);
    p.gamma = j.value("gamma", p.gamma);
    p.bypass = j.value("bypass", p.bypass);
}

Json toJson(const CurveParams& p) {
    return Json{{"knots", arrayToJson(p.knots)},
                {"contrast", p.contrast},
                {"gamma", p.gamma},
                {"bypass", p.bypass}};
}

void mergeSaturation(const Json& j, SaturationParams& p) {
    p.scale = j.value("scale", p.scale);
    p.bypass = j.value("bypass", p.bypass);
}

Json toJson(const SaturationParams& p) { return Json{{"scale", p.scale}, {"bypass", p.bypass}}; }

Json toJson(const WbDiag& d) {
    return Json{{"used", wbMethodToString(d.used)},
                {"avgRgb", arrayToJson(d.avgRgb)},
                {"gains", arrayToJson(d.gains)},
                {"confidence", d.confidence},
                {"explanation", d.explanation}};
}

Json toJson(const LevelsDiag& d) {
    return Json{{"blackPt", d.blackPt}, {"whitePt", d.whitePt}, {"inMin", d.inMin},
                {"inMax", d.inMax},     {"blackPct", d.blackPct}, {"whitePct", d.whitePct}};
}

Json toJson(const ClaheDiag& d) {
    return Json{
        {"clipLimit", d.clipLimit}, {"tilesX", d.tilesX}, {"tilesY", d.tilesY}, {"bins", d.bins}};
}

Json toJson(const StageTiming& t) {
    return Json{{"stage", t.stage}, {"ms", t.ms}, {"onGpu", t.onGpu}};
}

} // namespace

Expected<Params, JsonError> paramsFromJson(std::string_view json) {
    Json parsed;
    try {
        parsed = Json::parse(json);
    } catch (const Json::parse_error&) {
        return make_unexpected(JsonError::ParseFailed);
    }
    if (!parsed.is_object()) {
        return make_unexpected(JsonError::InvalidValue);
    }

    Params params;
    try {
        if (parsed.contains("wb")) {
            mergeWhiteBalance(parsed["wb"], params.wb);
        }
        if (parsed.contains("levels")) {
            mergeLevels(parsed["levels"], params.levels);
        }
        if (parsed.contains("clahe")) {
            mergeClahe(parsed["clahe"], params.clahe);
        }
        if (parsed.contains("curve")) {
            mergeCurve(parsed["curve"], params.curve);
        }
        if (parsed.contains("saturation")) {
            mergeSaturation(parsed["saturation"], params.saturation);
        }
        if (parsed.contains("schemaVersion")) {
            params.schemaVersion = parsed["schemaVersion"].get<std::uint32_t>();
        }
    } catch (const Json::exception&) {
        return make_unexpected(JsonError::InvalidValue);
    }
    return params;
}

std::string paramsToJson(const Params& params) {
    Json j{{"wb", toJson(params.wb)},
           {"levels", toJson(params.levels)},
           {"clahe", toJson(params.clahe)},
           {"curve", toJson(params.curve)},
           {"saturation", toJson(params.saturation)},
           {"schemaVersion", params.schemaVersion}};
    return j.dump();
}

std::string diagnosticsToJson(const Diagnostics& diag) {
    Json timings = Json::array();
    for (const StageTiming& timing : diag.timings) {
        timings.push_back(toJson(timing));
    }
    Json rgbAfter = Json::array();
    for (const Histogram256& hist : diag.rgbAfter) {
        rgbAfter.push_back(arrayToJson(hist));
    }

    Json j{{"wb", toJson(diag.wb)},
           {"levels", toJson(diag.levels)},
           {"clahe", toJson(diag.clahe)},
           {"lumaBefore", arrayToJson(diag.lumaBefore)},
           {"lumaAfter", arrayToJson(diag.lumaAfter)},
           {"rgbAfter", rgbAfter},
           {"timings", timings}};
    return j.dump();
}

} // namespace luma
