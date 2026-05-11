#pragma once
#include <optional>
#include <string>
#include <vector>

namespace arld::core {

enum class AircraftCategory {
    Warbird,
    JetFighter,
    HeavyTransport,
    Bomber,
    GeneralAviation,
    Aerobatic,
    Helicopter,
    BusinessJet,
};

enum class DisplayType {
    StaticDisplay,
    WarbirdHeritage,
    TaxiOnly,
    MilitaryStatic,
    HotRamp,
    MediaPhotoPlatform,
    RampShow,
};

struct AircraftLibraryEntry {
    std::string id;
    std::string displayName;
    std::string manufacturer;
    std::string model;
    std::string variant;
    AircraftCategory category        = AircraftCategory::GeneralAviation;
    float wingspanFt                 = 0.0f;
    float lengthFt                   = 0.0f;
    float tailHeightFt               = 0.0f;
    std::optional<float> propArcFt;       // only for piston/turboprop aircraft
    std::optional<float> rotorDiameterFt; // only for helicopters
    DisplayType defaultDisplayType   = DisplayType::StaticDisplay;
    std::string silhouetteSvg;            // filename only, e.g. "north-american-p51-d.svg"
    std::vector<std::string> dataSources;
};

} // namespace arld::core
