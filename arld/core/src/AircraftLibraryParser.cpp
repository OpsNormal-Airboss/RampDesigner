#include <arld/core/AircraftLibraryParser.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

namespace arld::core {

namespace {

AircraftCategory categoryFromString(const std::string& s) {
    if (s == "WARBIRD")           return AircraftCategory::Warbird;
    if (s == "JET_FIGHTER")       return AircraftCategory::JetFighter;
    if (s == "HEAVY_TRANSPORT")   return AircraftCategory::HeavyTransport;
    if (s == "BOMBER")            return AircraftCategory::Bomber;
    if (s == "GENERAL_AVIATION")  return AircraftCategory::GeneralAviation;
    if (s == "AEROBATIC")         return AircraftCategory::Aerobatic;
    if (s == "HELICOPTER")        return AircraftCategory::Helicopter;
    if (s == "BUSINESS_JET")      return AircraftCategory::BusinessJet;
    throw std::invalid_argument("Unknown AircraftCategory: " + s);
}

DisplayType displayTypeFromString(const std::string& s) {
    if (s == "STATIC_DISPLAY")      return DisplayType::StaticDisplay;
    if (s == "WARBIRD_HERITAGE")    return DisplayType::WarbirdHeritage;
    if (s == "TAXI_ONLY")           return DisplayType::TaxiOnly;
    if (s == "MILITARY_STATIC")     return DisplayType::MilitaryStatic;
    if (s == "HOT_RAMP")            return DisplayType::HotRamp;
    if (s == "MEDIA_PLATFORM")      return DisplayType::MediaPhotoPlatform;
    if (s == "RAMP_SHOW")           return DisplayType::RampShow;
    throw std::invalid_argument("Unknown DisplayType: " + s);
}

} // anonymous namespace

AircraftLibraryEntry AircraftLibraryParser::parseEntry(const std::string& jsonContent) {
    const auto j = json::parse(jsonContent);
    AircraftLibraryEntry e;
    e.id               = j.at("id").get<std::string>();
    e.displayName      = j.at("display_name").get<std::string>();
    e.manufacturer     = j.at("manufacturer").get<std::string>();
    e.model            = j.at("model").get<std::string>();
    e.variant          = j.value("variant", std::string{});
    e.category         = categoryFromString(j.at("category").get<std::string>());
    e.wingspanFt       = j.at("wingspan_ft").get<float>();
    e.lengthFt         = j.at("length_ft").get<float>();
    e.tailHeightFt     = j.at("tail_height_ft").get<float>();
    if (j.contains("prop_arc_ft"))
        e.propArcFt = j.at("prop_arc_ft").get<float>();
    if (j.contains("rotor_diameter_ft"))
        e.rotorDiameterFt = j.at("rotor_diameter_ft").get<float>();
    if (j.contains("min_turn_radius_ft"))
        e.minTurnRadiusFt = j.at("min_turn_radius_ft").get<float>();
    e.hasRetractableGear = j.value("has_retractable_gear", false);
    e.defaultDisplayType = displayTypeFromString(j.at("default_display_type").get<std::string>());
    e.silhouetteSvg    = j.at("silhouette_svg").get<std::string>();
    e.dataSources      = j.at("data_sources").get<std::vector<std::string>>();
    return e;
}

std::vector<std::string> AircraftLibraryParser::parseManifest(const std::string& jsonContent) {
    const auto j = json::parse(jsonContent);
    return j.at("entries").get<std::vector<std::string>>();
}

} // namespace arld::core
