#include <arld/core/LayoutDiffer.h>
#include <cmath>
#include <unordered_map>

namespace arld::core {

DeltaResult LayoutDiffer::diff(const std::vector<PlacedAircraft>& base,
                                const std::vector<PlacedAircraft>& compare) {
    DeltaResult result;

    // Build map from placement_id to aircraft for the base list
    std::unordered_map<std::string, const PlacedAircraft*> baseMap;
    baseMap.reserve(base.size());
    for (const auto& ac : base)
        baseMap[ac.placementId] = &ac;

    // Build map for compare list
    std::unordered_map<std::string, const PlacedAircraft*> compareMap;
    compareMap.reserve(compare.size());
    for (const auto& ac : compare)
        compareMap[ac.placementId] = &ac;

    // Walk compare list: find Added, Moved, Unchanged
    for (const auto& ac : compare) {
        auto it = baseMap.find(ac.placementId);
        if (it == baseMap.end()) {
            // In compare but not base → Added
            AircraftDelta d;
            d.aircraft = ac;
            d.change   = DeltaChange::Added;
            result.deltas.push_back(std::move(d));
        } else {
            const auto* baseAc = it->second;
            const float dx = ac.centerX - baseAc->centerX;
            const float dy = ac.centerY - baseAc->centerY;
            const float dist = std::sqrt(dx * dx + dy * dy);
            AircraftDelta d;
            d.aircraft = ac;
            if (dist > 0.01f) {
                d.change = DeltaChange::Moved;
                d.fromX  = baseAc->centerX;
                d.fromY  = baseAc->centerY;
            } else {
                d.change = DeltaChange::Unchanged;
            }
            result.deltas.push_back(std::move(d));
        }
    }

    // Walk base list: find Removed (in base but not compare)
    for (const auto& ac : base) {
        if (compareMap.find(ac.placementId) == compareMap.end()) {
            AircraftDelta d;
            d.aircraft = ac;
            d.change   = DeltaChange::Removed;
            result.deltas.push_back(std::move(d));
        }
    }

    return result;
}

} // namespace arld::core
