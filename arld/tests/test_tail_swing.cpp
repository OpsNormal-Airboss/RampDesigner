#include <arld/core/ClearanceEngine.h>
#include <arld/core/Config.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using arld::core::AircraftState;
using arld::core::ClearanceEngine;
using arld::core::DisplayType;

// Helper: build a minimal aircraft state for tail-swing tests.
static AircraftState makeTailDraggerState(const std::string& id,
                                           float cx, float cy,
                                           float wingspan, float length,
                                           float rotDeg = 0.0f,
                                           std::optional<float> minTurnRadius = std::nullopt,
                                           bool gearExtended = true) {
    AircraftState s;
    s.id              = id;
    s.centerX         = cx;
    s.centerY         = cy;
    s.wingspanFt      = wingspan;
    s.lengthFt        = length;
    s.rotationDeg     = rotDeg;
    s.displayType     = DisplayType::WarbirdHeritage;
    s.minTurnRadiusFt = minTurnRadius;
    s.gearExtended    = gearExtended;
    return s;
}

// ---------------------------------------------------------------------------
// Test 1: tailSwingPolygon returns empty for aircraft without minTurnRadiusFt
// ---------------------------------------------------------------------------
TEST_CASE("tailSwingPolygon: empty polygon when minTurnRadiusFt not set", "[tail_swing]") {
    const auto s = makeTailDraggerState("a", 0.0f, 0.0f, 32.0f, 24.0f);
    // minTurnRadiusFt is not set (nullopt by default)
    const auto poly = ClearanceEngine::tailSwingPolygon(s);
    REQUIRE(poly.size() == 0u);
}

// ---------------------------------------------------------------------------
// Test 2: tailSwingPolygon returns 16-vertex polygon for PT-17 (min_turn_radius_ft=18)
// ---------------------------------------------------------------------------
TEST_CASE("tailSwingPolygon: 16-vertex polygon for PT-17 tail-dragger", "[tail_swing]") {
    const auto s = makeTailDraggerState("pt17", 0.0f, 0.0f,
                                         32.17f, 24.42f,  // PT-17 dimensions
                                         0.0f,
                                         std::optional<float>(18.0f)); // min_turn_radius_ft
    const auto poly = ClearanceEngine::tailSwingPolygon(s);
    REQUIRE(poly.size() == 16u);
}

// ---------------------------------------------------------------------------
// Test 3: tail-swing polygon centre is near rear of aircraft (within 1 ft tolerance)
// ---------------------------------------------------------------------------
TEST_CASE("tailSwingPolygon: circle centre is at rear of aircraft", "[tail_swing]") {
    // Aircraft at origin, no rotation (rotation=0 → Y-down is behind).
    // Rear = (0, lengthFt/2) = (0, 12.21) for PT-17.
    const float length = 24.42f;
    const auto s = makeTailDraggerState("pt17", 0.0f, 0.0f, 32.17f, length,
                                         0.0f, std::optional<float>(18.0f));
    const auto poly = ClearanceEngine::tailSwingPolygon(s);
    REQUIRE(poly.size() == 16u);

    // Compute centroid of the polygon to find the circle centre.
    double sumX = 0.0, sumY = 0.0;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        sumX += CGAL::to_double(poly.vertex(i).x());
        sumY += CGAL::to_double(poly.vertex(i).y());
    }
    const double centreX = sumX / static_cast<double>(poly.size());
    const double centreY = sumY / static_cast<double>(poly.size());

    // Expected rear: (0, +length/2) = (0, 12.21)
    const double expectedX = 0.0;
    const double expectedY = static_cast<double>(length) / 2.0;

    REQUIRE(std::abs(centreX - expectedX) < 1.0);
    REQUIRE(std::abs(centreY - expectedY) < 1.0);
}

// ---------------------------------------------------------------------------
// Test 4: tail-swing polygon radius equals minTurnRadiusFt (±0.5 ft)
// ---------------------------------------------------------------------------
TEST_CASE("tailSwingPolygon: radius matches minTurnRadiusFt", "[tail_swing]") {
    const float turnRadius = 18.0f;
    const float length = 24.42f;
    const auto s = makeTailDraggerState("pt17", 0.0f, 0.0f, 32.17f, length,
                                         0.0f, std::optional<float>(turnRadius));
    const auto poly = ClearanceEngine::tailSwingPolygon(s);
    REQUIRE(poly.size() == 16u);

    // Compute centroid to find circle centre.
    double sumX = 0.0, sumY = 0.0;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        sumX += CGAL::to_double(poly.vertex(i).x());
        sumY += CGAL::to_double(poly.vertex(i).y());
    }
    const double cx = sumX / static_cast<double>(poly.size());
    const double cy = sumY / static_cast<double>(poly.size());

    // Check radius of each vertex to centroid.
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const double dx = CGAL::to_double(poly.vertex(i).x()) - cx;
        const double dy = CGAL::to_double(poly.vertex(i).y()) - cy;
        const double r = std::sqrt(dx * dx + dy * dy);
        REQUIRE(std::abs(r - static_cast<double>(turnRadius)) < 0.5);
    }
}

// ---------------------------------------------------------------------------
// Test 5: gearExtended=true produces larger clearance envelope than gearExtended=false
// ---------------------------------------------------------------------------
TEST_CASE("clearanceEnvelope: gear extended adds kGearExtendedAdditionFt to margin",
          "[tail_swing]") {
    // Same aircraft, same display type, same position.
    // Only difference: gearExtended true vs false.
    const float wingspan = 37.0f;
    const float length   = 32.0f;

    auto sGearDown = makeTailDraggerState("a", 0.0f, 0.0f, wingspan, length,
                                           0.0f, std::nullopt, true);
    sGearDown.displayType = DisplayType::StaticDisplay;

    auto sGearUp   = makeTailDraggerState("b", 0.0f, 0.0f, wingspan, length,
                                           0.0f, std::nullopt, false);
    sGearUp.displayType = DisplayType::StaticDisplay;

    const auto envDown = ClearanceEngine::clearanceEnvelope(sGearDown);
    const auto envUp   = ClearanceEngine::clearanceEnvelope(sGearUp);

    // Compute bounding box X-extent for each (proxy for size difference).
    double minXDown = 1e9, maxXDown = -1e9;
    double minXUp   = 1e9, maxXUp   = -1e9;
    for (std::size_t i = 0; i < envDown.size(); ++i) {
        double x = CGAL::to_double(envDown.vertex(i).x());
        if (x < minXDown) minXDown = x;
        if (x > maxXDown) maxXDown = x;
    }
    for (std::size_t i = 0; i < envUp.size(); ++i) {
        double x = CGAL::to_double(envUp.vertex(i).x());
        if (x < minXUp) minXUp = x;
        if (x > maxXUp) maxXUp = x;
    }

    const double widthDown = maxXDown - minXDown;
    const double widthUp   = maxXUp   - minXUp;

    // Gear down envelope must be larger by 2 * kGearExtendedAdditionFt (both sides).
    const double expectedDiff = 2.0 * static_cast<double>(arld::core::kGearExtendedAdditionFt);
    REQUIRE(std::abs((widthDown - widthUp) - expectedDiff) < 0.1);
}

// ---------------------------------------------------------------------------
// Test 6: clearanceEnvelope rear extent >= minTurnRadiusFt when it exceeds length/2
// ---------------------------------------------------------------------------
TEST_CASE("clearanceEnvelope: rear extent >= minTurnRadiusFt when larger than half-length",
          "[tail_swing]") {
    // PT-17: length 24.42 ft, half-length = 12.21 ft.
    // minTurnRadiusFt = 18.0 ft — larger than half-length, so envelope must extend to 18 ft rear.
    const float length = 24.42f;
    const float turnRadius = 18.0f;

    auto s = makeTailDraggerState("pt17", 0.0f, 0.0f, 32.17f, length,
                                   0.0f, std::optional<float>(turnRadius), false);
    // gear retracted so gear margin doesn't obscure the test
    s.displayType = DisplayType::StaticDisplay;

    const auto env = ClearanceEngine::clearanceEnvelope(s);

    // With rotation=0, the rear is at +Y. Find max Y of envelope.
    double maxY = -1e9;
    for (std::size_t i = 0; i < env.size(); ++i) {
        double y = CGAL::to_double(env.vertex(i).y());
        if (y > maxY) maxY = y;
    }

    // The rear of the envelope (maxY) must be >= minTurnRadiusFt from centre (0,0).
    REQUIRE(maxY >= static_cast<double>(turnRadius));
}
