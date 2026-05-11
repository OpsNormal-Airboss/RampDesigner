#include <arld/core/ClearanceEngine.h>
#include <arld/core/Config.h>
#include <CGAL/squared_distance_2.h>
#include <cmath>
#include <limits>
#include <numbers>

namespace arld::core {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static double toRad(double deg) { return deg * M_PI / 180.0; }

// Build a rotated axis-aligned rectangle centred at (cx, cy).
// halfW and halfH are the half-dimensions of the expanded rectangle.
static Polygon2 makeRotatedRect(double cx, double cy,
                                 double halfW, double halfH,
                                 double angleDeg) {
    const double a = toRad(angleDeg);
    const double cosA = std::cos(a);
    const double sinA = std::sin(a);

    // Four corners in local space (before rotation).
    const double xs[4] = { +halfW, -halfW, -halfW, +halfW };
    const double ys[4] = { +halfH, +halfH, -halfH, -halfH };

    Polygon2 poly;
    for (int i = 0; i < 4; ++i) {
        // Qt rotation is clockwise-positive: rotate +angle clockwise.
        const double rx = xs[i] * cosA - ys[i] * sinA;
        const double ry = xs[i] * sinA + ys[i] * cosA;
        poly.push_back(Point2(cx + rx, cy + ry));
    }
    return poly;
}

// ---------------------------------------------------------------------------
// ClearanceEngine public API
// ---------------------------------------------------------------------------
float ClearanceEngine::requiredClearanceFt(DisplayType dt,
                                            std::optional<float> propArcFt,
                                            const ClearanceRuleSet& rules) {
    switch (dt) {
        case DisplayType::StaticDisplay:
            return rules.staticDisplayWingtipFt;
        case DisplayType::WarbirdHeritage:
            // prop arc radius + additional bonus margin (or just bonus + static if no arc)
            return (propArcFt.value_or(0.0f)) + rules.warbirdPropArcBonusFt;
        case DisplayType::TaxiOnly:
            return rules.taxiOnlyCorridorFt;
        case DisplayType::MilitaryStatic:
            return rules.militaryStaticStandoffFt;
        case DisplayType::HotRamp:
            return rules.hotRampStandoffFt;
        case DisplayType::MediaPhotoPlatform:
            return rules.mediaPhotoPlatformFt;
        case DisplayType::RampShow:
            return rules.rampShowCrowdLineFt;
    }
    return rules.staticDisplayWingtipFt;
}

Polygon2 ClearanceEngine::aircraftFootprint(const AircraftState& s) {
    return makeRotatedRect(
        static_cast<double>(s.centerX),
        static_cast<double>(s.centerY),
        static_cast<double>(s.wingspanFt) / 2.0,
        static_cast<double>(s.lengthFt) / 2.0,
        static_cast<double>(s.rotationDeg));
}

Polygon2 ClearanceEngine::clearanceEnvelope(const AircraftState& s,
                                             const ClearanceRuleSet& rules) {
    double margin = static_cast<double>(
        requiredClearanceFt(s.displayType, s.propArcFt, rules));

    // Add gear-extended clearance bonus when gear is down.
    if (s.gearExtended) {
        margin += static_cast<double>(kGearExtendedAdditionFt);
    }

    double halfW = static_cast<double>(s.wingspanFt) / 2.0 + margin;
    double halfH = static_cast<double>(s.lengthFt) / 2.0 + margin;

    // If tail-swing radius is larger than the rear half of the envelope,
    // extend the rear half to accommodate the full tail-swing arc.
    // The "rear" is at +halfH along the local Y axis (Y-down = behind at rot=0).
    if (s.minTurnRadiusFt.has_value()) {
        const double tailSwingR = static_cast<double>(*s.minTurnRadiusFt);
        if (tailSwingR > halfH) {
            halfH = tailSwingR;
        }
    }

    return makeRotatedRect(
        static_cast<double>(s.centerX),
        static_cast<double>(s.centerY),
        halfW,
        halfH,
        static_cast<double>(s.rotationDeg));
}

Polygon2 ClearanceEngine::tailSwingPolygon(const AircraftState& s) {
    if (!s.minTurnRadiusFt.has_value()) {
        return Polygon2{};  // empty polygon — no tail-swing arc
    }

    const double radius = static_cast<double>(*s.minTurnRadiusFt);
    const double angleRad = static_cast<double>(s.rotationDeg) * std::numbers::pi / 180.0;
    const double cosA = std::cos(angleRad);
    const double sinA = std::sin(angleRad);

    // Centre of the tail-swing circle = rear of aircraft.
    // Rear is at local (0, +lengthFt/2) rotated into scene space.
    const double rearLocalY = static_cast<double>(s.lengthFt) / 2.0;
    // Qt clockwise rotation: scene_x = local_x*cos - local_y*sin; scene_y = local_x*sin + local_y*cos
    const double circleCx = static_cast<double>(s.centerX) + (-rearLocalY * sinA);
    const double circleCy = static_cast<double>(s.centerY) + ( rearLocalY * cosA);

    // Approximate the circle with 16 vertices.
    constexpr int kPoints = 16;
    Polygon2 poly;
    for (int i = 0; i < kPoints; ++i) {
        const double theta = 2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(kPoints);
        poly.push_back(Point2(circleCx + radius * std::cos(theta),
                              circleCy + radius * std::sin(theta)));
    }
    return poly;
}

// Minimum signed gap between two convex polygons.
// Returns -1.0f if they overlap (any vertex of one is inside or on the
// boundary of the other — covers the partial-overlap "edge-crossing" case).
float ClearanceEngine::minSeparationFt(const Polygon2& a, const Polygon2& b) {
    // Check all vertices of each polygon for containment in the other.
    // Using != ON_UNBOUNDED_SIDE catches both interior points AND boundary
    // contacts, which covers the case where edges cross without any vertex
    // being strictly interior.
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (a.bounded_side(b.vertex(i)) != CGAL::ON_UNBOUNDED_SIDE)
            return -1.0f;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (b.bounded_side(a.vertex(i)) != CGAL::ON_UNBOUNDED_SIDE)
            return -1.0f;
    }

    // Minimum edge-to-edge distance across all pairs.
    FT minSq(std::numeric_limits<double>::max());
    for (auto ei = a.edges_begin(); ei != a.edges_end(); ++ei) {
        for (auto ej = b.edges_begin(); ej != b.edges_end(); ++ej) {
            FT d = CGAL::squared_distance(*ei, *ej);
            if (d < minSq) minSq = d;
        }
    }

    return static_cast<float>(std::sqrt(CGAL::to_double(minSq)));
}

std::vector<ViolationResult> ClearanceEngine::detectViolations(
    const std::vector<AircraftState>& aircraft,
    const ClearanceRuleSet& rules) {

    std::vector<ViolationResult> results;

    // Pre-build footprint polygons once.
    std::vector<Polygon2> footprints;
    footprints.reserve(aircraft.size());
    for (const auto& s : aircraft)
        footprints.push_back(aircraftFootprint(s));

    const std::size_t n = aircraft.size();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            const float reqA = requiredClearanceFt(aircraft[i].displayType,
                                                    aircraft[i].propArcFt,
                                                    rules);
            const float reqB = requiredClearanceFt(aircraft[j].displayType,
                                                    aircraft[j].propArcFt,
                                                    rules);
            const float required = std::max(reqA, reqB);

            const float sep = minSeparationFt(footprints[i], footprints[j]);

            ClearanceSeverity severity;
            if (sep < 0.0f || sep < required) {
                severity = ClearanceSeverity::Violation;
            } else if (sep < required * 1.2f) {
                // Advisory: within 20% margin above the minimum required gap.
                severity = ClearanceSeverity::Advisory;
            } else {
                continue; // Clear — omit from results.
            }

            results.push_back({aircraft[i].id, aircraft[j].id,
                               severity, sep, required});
        }
    }

    return results;
}

} // namespace arld::core
