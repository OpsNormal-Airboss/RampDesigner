#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <arld/core/ClearanceRuleSet.h>
#include <arld/core/GeomTypes.h>
#include <optional>
#include <string>
#include <vector>

namespace arld::core {

enum class ClearanceSeverity { Clear, Advisory, Violation, Overridden };

// Snapshot of one aircraft's spatial state for clearance computation.
struct AircraftState {
    std::string id;
    float wingspanFt  = 0.0f;
    float lengthFt    = 0.0f;
    float centerX     = 0.0f;    // scene feet; x increases right
    float centerY     = 0.0f;    // scene feet; y increases down (Qt convention)
    float rotationDeg = 0.0f;    // clockwise-positive (Qt convention)
    DisplayType displayType = DisplayType::StaticDisplay;
    std::optional<float> propArcFt;
};

// One result per aircraft pair that is not fully clear.
struct ViolationResult {
    std::string idA;
    std::string idB;
    ClearanceSeverity severity = ClearanceSeverity::Clear;
    float separationFt = 0.0f;  // measured gap between hulls (negative = overlapping)
    float requiredFt   = 0.0f;  // minimum required gap for this pair
};

class ClearanceEngine {
public:
    // Minimum hull-to-hull gap required by the more restrictive of the pair's display types.
    static float requiredClearanceFt(DisplayType dt,
                                     std::optional<float> propArcFt = std::nullopt,
                                     const ClearanceRuleSet& rules = ClearanceRuleSet::faaCoW());

    // Rotated rectangle polygon for the physical aircraft footprint.
    // Coordinate origin is the scene origin; 1 unit = 1 ft.
    static Polygon2 aircraftFootprint(const AircraftState& s);

    // Expanded polygon representing the required clearance zone around one aircraft.
    static Polygon2 clearanceEnvelope(const AircraftState& s,
                                      const ClearanceRuleSet& rules = ClearanceRuleSet::faaCoW());

    // Pairwise violation detection. Returns one entry per pair where severity != Clear.
    // Advisory = within 20 % of the required gap. Violation = below the required gap.
    static std::vector<ViolationResult> detectViolations(
        const std::vector<AircraftState>& aircraft,
        const ClearanceRuleSet& rules = ClearanceRuleSet::faaCoW());

private:
    // Minimum gap between two convex polygons (negative if overlapping).
    static float minSeparationFt(const Polygon2& a, const Polygon2& b);
};

} // namespace arld::core
