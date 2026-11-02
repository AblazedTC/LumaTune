// T-3 (PLAN.md §11). Fixtures/expected outputs are generated once from
// legacy V1 by tools/gen_golden.py (PLAN.md P0.2) and checked in - this
// file only compares, never regenerates (AGENTS.md §9/§14).
//
// Tolerance was measured, not assumed (PLAN.md's P1.6/T-3 notes): a
// Python port of this exact chain against all fixtures found the
// originally-planned PSNR>=45dB/max|dE00|<=1 unachievable, because legacy
// quantises to uint8 *before* AutoLevels/CLAHE even run while ours stays
// float32 throughout, and CLAHE's local contrast enhancement amplifies
// that nonlinearly. Color-cast-stress fixtures additionally hit P1.4's
// accepted WB G-normalisation-vs-grand-mean difference, which isn't a bug
// (WhiteBalanceTest.cpp already covers WB's own correctness via gain
// ratios) but makes full-chain pixel comparison unfair for them. So:
//   - adobergb_tagged, warm_cast, cool_cast, dominant_colour: smoke-check
//     only (valid shape/range, no NaN) - see above for why.
//   - the other 8 fixtures x 4 param sets: real PSNR/mean-dE00 check, at
//     the measured-and-padded bound from PLAN.md's T-3 row.
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "ImageMetrics.h"
#include "engine/Pipeline.h"
#include "io/Decoder.h"

namespace {

struct Fixture {
    std::string stem;
    std::string ext;
    bool smokeOnly; // see file header for why
};

// All 12 golden fixtures (tests/golden/fixtures/SOURCES.md).
const std::array<Fixture, 12> kFixtures{{
    {"adobergb_tagged", ".jpg", true},
    {"warm_cast", ".jpg", true},
    {"cool_cast", ".jpg", true},
    {"dominant_colour", ".png", true},
    {"neutral", ".png", false},
    {"high_key", ".png", false},
    {"low_key", ".png", false},
    {"exif_rotated", ".jpg", false},
    {"high_dynamic_range", ".png", false},
    {"low_dynamic_range", ".png", false},
    {"sample", ".webp", false},
    {"sixteen_bit", ".png", false},
}};

struct ParamSet {
    std::string name;
    float blackPct;
    float whitePct;
    float claheClip;
    int claheTile;
};

// Exactly tools/gen_golden.py's PARAM_SETS.
const std::array<ParamSet, 4> kParamSets{{
    {"default", 0.5f, 99.5f, 2.0f, 8},
    {"subtle", 1.0f, 99.0f, 1.0f, 8},
    {"punchy", 0.1f, 99.9f, 4.0f, 8},
    {"fine_tile", 0.5f, 99.5f, 2.0f, 16},
}};

// Golden tests run with clahe.bins=256 (PLAN.md T-3: "During Phase 1 the
// C++ pipeline runs with clahe.bins=256") - not the 1024 shipping default.
luma::Params toLumaParams(const ParamSet& set) {
    luma::Params params;
    params.levels.blackPct = set.blackPct;
    params.levels.whitePct = set.whitePct;
    params.clahe.clipLimit = set.claheClip;
    params.clahe.tilesX = set.claheTile;
    params.clahe.tilesY = set.claheTile;
    params.clahe.bins = 256;
    return params;
}

cv::Mat toRgbU8(const luma::ImageBuffer& image) {
    cv::Mat u8;
    image.mat().convertTo(u8, CV_8U, 255.0);
    return u8;
}

cv::Mat loadExpectedRgb(const std::string& path) {
    cv::Mat bgr = cv::imread(path, cv::IMREAD_COLOR);
    REQUIRE_FALSE(bgr.empty());
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    return rgb;
}

void checkValidOutput(const luma::ImageBuffer& image, const cv::Size& expectedSize) {
    CHECK(image.mat().size() == expectedSize);
    double minVal = 0.0;
    double maxVal = 0.0;
    cv::minMaxLoc(image.mat().reshape(1), &minVal, &maxVal);
    CHECK(minVal >= 0.0);
    CHECK(maxVal <= 1.0);
    CHECK(cv::checkRange(image.mat())); // false on any NaN/Inf
}

} // namespace

TEST_CASE("Golden/pipeline matches legacy V1 within the measured tolerance",
          "[golden]") {
    const std::string fixturesDir = std::string(LUMA_REPO_ROOT) + "/tests/golden/fixtures/";
    const std::string expectedDir = std::string(LUMA_REPO_ROOT) + "/tests/golden/expected/";

    for (const Fixture& fixture : kFixtures) {
        const std::string path = fixturesDir + fixture.stem + fixture.ext;
        auto decoded = luma::decodeImage(path);
        REQUIRE(decoded.has_value());

        for (const ParamSet& set : kParamSets) {
            auto result = luma::runPipeline(*decoded, toLumaParams(set));
            REQUIRE(result.has_value());
            checkValidOutput(result->image, cv::Size(decoded->width, decoded->height));

            if (fixture.smokeOnly) {
                continue;
            }

            const cv::Mat ours = toRgbU8(result->image);
            const cv::Mat expected =
                loadExpectedRgb(expectedDir + fixture.stem + "__" + set.name + ".png");
            REQUIRE(ours.size() == expected.size());

            const double p = luma::test::psnr(ours, expected);
            const double meanDe = luma::test::meanDeltaE2000(ours, expected);
            INFO("fixture=" << fixture.stem << " paramSet=" << set.name << " PSNR=" << p
                             << "dB meanDeltaE00=" << meanDe);
            CHECK(p >= 22.0);
            CHECK(meanDe <= 9.0);
        }
    }
}
