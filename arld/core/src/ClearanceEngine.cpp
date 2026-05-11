#include <arld/core/ClearanceEngine.h>
#include <arld/core/Config.h>
#include <CGAL/squared_distance_2.h>
#include <cmath>
#include <limits>

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
                                            std::optional<float> propArcFt) {
    switch (dt) {
        case DisplayType::StaticDisplay:
            return kStaticDisplayWingtip;
        case DisplayType::WarbirdHeritage:
            // prop arc + additional margin (or just the standard wingtip if no arc)
            return (propArcFt.value_or(0.0f)) + kWarbirdPropArcAddition
                   + kStaticDisplayWingtip;
        case DisplayType::TaxiOnly:
            return kTaxiOnlyCorridor;
        case DisplayType::MilitaryStatic:
            return kMilitaryStaticStandoff;
        case DisplayType::HotRamp:
            return kHotRampNoSmoking;
        case DisplayType::MediaPhotoPlatform:
            return kMediaPlatformBarrier;
        case DisplayType::RampShow:
            return kRampShowCrowdLine;
    }
    return kStaticDisplayWingtip;
}

Polygon2 ClearanceEngine::aircraftFootprint(const AircraftState& s) {
    return makeRotatedRect(
        static_cast<double>(s.centerX),
        static_cast<double>(s.centerY),
        static_cast<double>(s.wingspanFt) / 2.0,
        static_cast<double>(s.lengthFt) / 2.0,
        static_cast<double>(s.rotationDeg));
}

Polygon2 ClearanceEngine::clearanceEnvelope(const AircraftState& s) {
    const double margin = static_cast<double>(
        requiredClearanceFt(s.displayType, s.propArcFt));
    return makeRotatedRect(
        static_cast<double>(s.centerX),
        static_cast<double>(s.centerY),
        static_cast<double>(s.wingspanFt) / 2.0 + margin,
        static_cast<double>(s.lengthFt) / 2.0 + margin,
        static_cast<double>(s.rotationDeg));
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
    const std::vector<AircraftState>& aircraft) {

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
                                                    aircraft[i].propArcFt);
            const float reqB = requiredClearanceFt(aircraft[j].displayType,
                                                    aircraft[j].propArcFt);
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
