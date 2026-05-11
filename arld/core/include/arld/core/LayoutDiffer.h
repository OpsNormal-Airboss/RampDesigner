#pragma once
#include <arld/core/ProjectFile.h>
#include <string>
#include <vector>

namespace arld::core {

enum class DeltaChange { Unchanged, Added, Removed, Moved };

struct AircraftDelta {
    PlacedAircraft aircraft;
    DeltaChange    change;
    // For Moved: original position from base
    float          fromX = 0.f;
    float          fromY = 0.f;
};

struct DeltaResult {
    std::vector<AircraftDelta> deltas;
};

class LayoutDiffer {
public:
    /// Diff base vs. compare by placement_id.
    /// Aircraft in compare but not base → Added.
    /// Aircraft in base but not compare → Removed.
    /// Aircraft in both with different center (> 0.01 ft) → Moved.
    /// Aircraft in both, same position (within 0.01 ft) → Unchanged.
    static DeltaResult diff(const std::vector<PlacedAircraft>& base,
                            const std::vector<PlacedAircraft>& compare);
};

} // namespace arld::core
