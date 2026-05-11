#include <arld/core/ClearanceEngine.h>
#include <arld/core/Config.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <cmath>

using arld::core::AircraftState;
using arld::core::ClearanceEngine;
using arld::core::ClearanceSeverity;
using arld::core::DisplayType;

// Helper: build a minimal aircraft state.
static AircraftState makeState(const std::string& id,
                                float cx, float cy,
                                float wingspan, float length,
                                DisplayType dt = DisplayType::StaticDisplay,
                                float rotation = 0.0f,
                                std::optional<float> propArc = std::nullopt) {
    AircraftState s;
    s.id          = id;
    s.centerX     = cx;
    s.centerY     = cy;
    s.wingspanFt  = wingspan;
    s.lengthFt    = length;
    s.rotationDeg = rotation;
    s.displayType = dt;
    s.propArcFt   = propArc;
    return s;
}

// ---------------------------------------------------------------------------
// Scenario 1: Two aircraft well separated — no violation
// ---------------------------------------------------------------------------
TEST_CASE("Scenario 1: Well-separated static-display aircraft — no violation",
          "[clearance]") {
    // P-51D wingspan ~37 ft; place aircraft 500 ft apart.
    const auto a = makeState("a", 0.0f,   0.0f,   37.0f, 32.0f);
    const auto b = makeState("b", 500.0f, 0.0f,   37.0f, 32.0f);

    const auto violations = ClearanceEngine::detectViolations({a, b});
    REQUIRE(violations.empty());
}

// ---------------------------------------------------------------------------
// Scenario 2: Two static-display aircraft too close — violation
// ---------------------------------------------------------------------------
TEST_CASE("Scenario 2: Static-display aircraft closer than 25 ft gap — violation",
          "[clearance]") {
    // Required gap = 25 ft. Place aircraft 10 ft apart hull-to-hull.
    // Each has 37 ft wingspan → each hull extends 18.5 ft from centre.
    // To get 10 ft gap: centres 10 + 18.5 + 18.5 = 47 ft apart.
    const auto a = makeState("a",  0.0f, 0.0f, 37.0f, 32.0f);
    const auto b = makeState("b", 47.0f, 0.0f, 37.0f, 32.0f);

    const auto violations = ClearanceEngine::detectViolations({a, b});
    REQUIRE(!violations.empty());
    REQUIRE(violations[0].severity == ClearanceSeverity::Violation);
    REQUIRE(violations[0].requiredFt == arld::core::kStaticDisplayWingtip);
    // Measured separation ≈ 10 ft (within float tolerance).
    REQUIRE(violations[0].separationFt < arld::core::kStaticDisplayWingtip);
}

// ---------------------------------------------------------------------------
// Scenario 3: Warbird-heritage aircraft violating prop-arc clearance
// ---------------------------------------------------------------------------
TEST_CASE("Scenario 3: Warbird with prop arc — violation when inside prop arc zone",
          "[clearance]") {
    // P-51D: wingspan 37 ft, prop arc ~8 ft radius.
    // Required = propArc + kWarbirdPropArcAddition + kStaticDisplayWingtip
    //          = 8 + 10 + 25 = 43 ft
    // Place aircraft 30 ft hull-to-hull apart → violation.
    const float propArc = 8.0f;
    const float required = ClearanceEngine::requiredClearanceFt(
        DisplayType::WarbirdHeritage, propArc);

    const auto a = makeState("a", 0.0f, 0.0f, 37.0f, 32.0f,
                              DisplayType::WarbirdHeritage, 0.0f, propArc);
    // 30 ft hull-to-hull gap → centres 30 + 18.5 + 18.5 = 67 ft apart.
    const auto b = makeState("b", 67.0f, 0.0f, 37.0f, 32.0f,
                              DisplayType::WarbirdHeritage, 0.0f, propArc);

    const auto violations = ClearanceEngine::detectViolations({a, b});
    REQUIRE(!violations.empty());
    REQUIRE(violations[0].severity == ClearanceSeverity::Violation);
    REQUIRE(violations[0].requiredFt == required);
    REQUIRE(violations[0].separationFt < required);
}

// ---------------------------------------------------------------------------
// Scenario 4: Military-static standoff violation
// ---------------------------------------------------------------------------
TEST_CASE("Scenario 4: Military-static aircraft inside 50 ft standoff — violation",
          "[clearance]") {
    // Required gap = 50 ft.
    // F-16 wingspan ~31 ft. Place them 20 ft hull-to-hull.
    // Centres: 20 + 15.5 + 15.5 = 51 ft.
    const auto a = makeState("a",  0.0f, 0.0f, 31.0f, 49.0f,
                              DisplayType::MilitaryStatic);
    const auto b = makeState("b", 51.0f, 0.0f, 31.0f, 49.0f,
                              DisplayType::MilitaryStatic);

    const auto violations = ClearanceEngine::detectViolations({a, b});
    REQUIRE(!violations.empty());
    const auto& v = violations[0];
    REQUIRE(v.severity == ClearanceSeverity::Violation);
    REQUIRE(v.requiredFt == arld::core::kMilitaryStaticStandoff);
    REQUIRE(v.separationFt < arld::core::kMilitaryStaticStandoff);
}

// ---------------------------------------------------------------------------
// Scenario 5: Physical overlap between two large aircraft — violation
// ---------------------------------------------------------------------------
TEST_CASE("Scenario 5: Overlapping aircraft footprints — violation", "[clearance]") {
    // C-17: wingspan 170 ft, length 174 ft. Place at same position.
    const auto a = makeState("a", 0.0f, 0.0f,   170.0f, 174.0f);
    const auto b = makeState("b", 20.0f, 0.0f,  170.0f, 174.0f);

    const auto violations = ClearanceEngine::detectViolations({a, b});
    REQUIRE(!violations.empty());
    REQUIRE(violations[0].severity == ClearanceSeverity::Violation);
    // Overlapping footprints → negative separation.
    REQUIRE(violations[0].separationFt < 0.0f);
}

// ---------------------------------------------------------------------------
// Scenario 6: Advisory zone — within 20 % buffer above required gap
// ---------------------------------------------------------------------------
TEST_CASE("Scenario 6: Aircraft in advisory zone — advisory severity", "[clearance]") {
    // Required gap = 25 ft. Place at 28 ft hull-to-hull (within 25 * 1.2 = 30 ft buffer).
    // Centres: 28 + 18.5 + 18.5 = 65 ft.
    const auto a = makeState("a",  0.0f, 0.0f, 37.0f, 32.0f);
    const auto b = makeState("b", 65.0f, 0.0f, 37.0f, 32.0f);

    const auto violations = ClearanceEngine::detectViolations({a, b});
    REQUIRE(!violations.empty());
    REQUIRE(violations[0].severity == ClearanceSeverity::Advisory);
}

// ---------------------------------------------------------------------------
// Scenario 7: Rotated aircraft clearance check
// ---------------------------------------------------------------------------
TEST_CASE("Scenario 7: Rotated aircraft clearance — violation detected", "[clearance]") {
    // Rotate both aircraft 45 degrees. Their footprints are diamond-shaped
    // in scene space. Place close enough to still violate.
    const auto a = makeState("a",  0.0f, 0.0f, 37.0f, 32.0f,
                              DisplayType::StaticDisplay, 45.0f);
    const auto b = makeState("b", 40.0f, 0.0f, 37.0f, 32.0f,
                              DisplayType::StaticDisplay, 45.0f);

    const auto violations = ClearanceEngine::detectViolations({a, b});
    REQUIRE(!violations.empty());
}

// ---------------------------------------------------------------------------
// Scenario 8: requiredClearanceFt values are consistent with Config.h constants
// ---------------------------------------------------------------------------
TEST_CASE("requiredClearanceFt matches Config constants", "[clearance]") {
    using DT = DisplayType;
    REQUIRE(ClearanceEngine::requiredClearanceFt(DT::StaticDisplay)
            == arld::core::kStaticDisplayWingtip);
    REQUIRE(ClearanceEngine::requiredClearanceFt(DT::TaxiOnly)
            == arld::core::kTaxiOnlyCorridor);
    REQUIRE(ClearanceEngine::requiredClearanceFt(DT::MilitaryStatic)
            == arld::core::kMilitaryStaticStandoff);
    REQUIRE(ClearanceEngine::requiredClearanceFt(DT::HotRamp)
            == arld::core::kHotRampNoSmoking);
    REQUIRE(ClearanceEngine::requiredClearanceFt(DT::RampShow)
            == arld::core::kRampShowCrowdLine);
    REQUIRE(ClearanceEngine::requiredClearanceFt(DT::MediaPhotoPlatform)
            == arld::core::kMediaPlatformBarrier);
    // WarbirdHeritage with no prop arc.
    const float warbirdNoArc = ClearanceEngine::requiredClearanceFt(
        DT::WarbirdHeritage, std::nullopt);
    REQUIRE(warbirdNoArc == arld::core::kWarbirdPropArcAddition
                            + arld::core::kStaticDisplayWingtip);
}

// ---------------------------------------------------------------------------
// Performance benchmark: 200 aircraft O(N²) evaluation must be < 50 ms
// ---------------------------------------------------------------------------
TEST_CASE("bench_clearance_200: detectViolations with 200 aircraft", "[.bench]") {
    // Arrange 200 aircraft in a 10x20 grid with 300 ft spacing.
    // Most pairs will be far apart (Clear), some nearby pairs will be Advisory/Violation.
    std::vector<AircraftState> aircraft;
    aircraft.reserve(200);
    int idx = 0;
    for (int row = 0; row < 10; ++row) {
        for (int col = 0; col < 20; ++col) {
            // Vary spacing so some adjacent pairs are in violation.
            float spacing = (idx % 7 == 0) ? 30.0f : 300.0f;  // 30 ft = violation
            aircraft.push_back(makeState(
                "a" + std::to_string(idx),
                static_cast<float>(col) * spacing,
                static_cast<float>(row) * 300.0f,
                37.0f, 32.0f));
            ++idx;
        }
    }

    BENCHMARK("detectViolations 200 aircraft") {
        return ClearanceEngine::detectViolations(aircraft);
    };
}
