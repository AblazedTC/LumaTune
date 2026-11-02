#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "engine/ParamsJson.h"

using Catch::Approx;

TEST_CASE("ParamsJson/round-trips every field through toJson+fromJson",
          "[unit][engine][paramsjson]") {
    luma::Params params;
    params.wb.method = luma::WbMethod::ShadesOfGray;
    params.wb.strength = 0.75f;
    params.wb.gainMin = 0.4f;
    params.wb.gainMax = 2.5f;
    params.wb.excludeLow = 0.01f;
    params.wb.excludeHigh = 0.99f;
    params.wb.sogNorm = 4.0f;
    params.wb.bypass = true;
    params.levels.blackPct = 1.0f;
    params.levels.whitePct = 98.0f;
    params.levels.bypass = true;
    params.clahe.bins = 512;
    params.clahe.clipLimit = 3.0f;
    params.clahe.tilesX = 4;
    params.clahe.tilesY = 6;
    params.clahe.bypass = true;
    params.curve.knots[3] = 42.0f;
    params.curve.contrast = 0.2f;
    params.curve.gamma = 1.1f;
    params.curve.bypass = false;
    params.saturation.scale = 1.5f;
    params.saturation.bypass = false;
    params.schemaVersion = 7;

    const std::string json = luma::paramsToJson(params);
    auto roundTripped = luma::paramsFromJson(json);
    REQUIRE(roundTripped.has_value());

    CHECK(roundTripped->wb.method == luma::WbMethod::ShadesOfGray);
    CHECK(roundTripped->wb.strength == Approx(0.75f));
    CHECK(roundTripped->wb.gainMin == Approx(0.4f));
    CHECK(roundTripped->wb.gainMax == Approx(2.5f));
    CHECK(roundTripped->wb.excludeLow == Approx(0.01f));
    CHECK(roundTripped->wb.excludeHigh == Approx(0.99f));
    CHECK(roundTripped->wb.sogNorm == Approx(4.0f));
    CHECK(roundTripped->wb.bypass);
    CHECK(roundTripped->levels.blackPct == Approx(1.0f));
    CHECK(roundTripped->levels.whitePct == Approx(98.0f));
    CHECK(roundTripped->levels.bypass);
    CHECK(roundTripped->clahe.bins == 512);
    CHECK(roundTripped->clahe.clipLimit == Approx(3.0f));
    CHECK(roundTripped->clahe.tilesX == 4);
    CHECK(roundTripped->clahe.tilesY == 6);
    CHECK(roundTripped->clahe.bypass);
    CHECK(roundTripped->curve.knots[3] == Approx(42.0f));
    CHECK(roundTripped->curve.contrast == Approx(0.2f));
    CHECK(roundTripped->curve.gamma == Approx(1.1f));
    CHECK_FALSE(roundTripped->curve.bypass);
    CHECK(roundTripped->saturation.scale == Approx(1.5f));
    CHECK_FALSE(roundTripped->saturation.bypass);
    CHECK(roundTripped->schemaVersion == 7);
}

TEST_CASE("ParamsJson/a partial JSON object keeps defaults for everything else",
          "[unit][engine][paramsjson]") {
    auto result = luma::paramsFromJson(R"({"wb": {"strength": 0.5}})");
    REQUIRE(result.has_value());

    CHECK(result->wb.strength == Approx(0.5f));
    CHECK(result->wb.method == luma::WbMethod::GrayWorld); // default, not touched
    const luma::LevelsParams defaultLevels;
    CHECK(result->levels.blackPct == Approx(defaultLevels.blackPct));
    CHECK(result->levels.whitePct == Approx(defaultLevels.whitePct));
}

TEST_CASE("ParamsJson/an empty object is just all defaults", "[unit][engine][paramsjson]") {
    auto result = luma::paramsFromJson("{}");
    REQUIRE(result.has_value());

    const luma::Params defaults;
    CHECK(result->wb.method == defaults.wb.method);
    CHECK(result->clahe.bins == defaults.clahe.bins);
    CHECK(result->schemaVersion == defaults.schemaVersion);
}

TEST_CASE("ParamsJson/malformed JSON fails cleanly instead of crashing",
          "[unit][engine][paramsjson]") {
    auto result = luma::paramsFromJson("{not valid json");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == luma::JsonError::ParseFailed);
}

TEST_CASE("ParamsJson/a JSON value that isn't an object is rejected",
          "[unit][engine][paramsjson]") {
    auto result = luma::paramsFromJson("[1, 2, 3]");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == luma::JsonError::InvalidValue);
}

TEST_CASE("ParamsJson/a wrong-shaped field fails cleanly rather than crashing",
          "[unit][engine][paramsjson]") {
    auto result = luma::paramsFromJson(R"({"wb": "not an object"})");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == luma::JsonError::InvalidValue);
}

TEST_CASE("ParamsJson/diagnosticsToJson produces parseable, non-empty JSON",
          "[unit][engine][paramsjson]") {
    luma::Diagnostics diag;
    diag.wb.explanation = "test";
    diag.timings.push_back(luma::StageTiming{"toLinear", 1.5, false});

    const std::string json = luma::diagnosticsToJson(diag);
    CHECK_FALSE(json.empty());
    CHECK(json.find("\"toLinear\"") != std::string::npos);
    CHECK(json.find("\"test\"") != std::string::npos);
}
