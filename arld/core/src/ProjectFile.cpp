#include <arld/core/ProjectFile.h>
#include <arld/core/ClearanceRuleSet.h>
#include <nlohmann/json.hpp>
#include <array>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace arld::core {

// ---------------------------------------------------------------------------
// DisplayType ↔ string
// ---------------------------------------------------------------------------
std::string displayTypeToString(DisplayType dt) {
    switch (dt) {
        case DisplayType::StaticDisplay:      return "static_display";
        case DisplayType::WarbirdHeritage:    return "warbird_heritage";
        case DisplayType::TaxiOnly:           return "taxi_only";
        case DisplayType::MilitaryStatic:     return "military_static";
        case DisplayType::HotRamp:            return "hot_ramp";
        case DisplayType::MediaPhotoPlatform: return "media_photo_platform";
        case DisplayType::RampShow:           return "ramp_show";
    }
    return "static_display"; // unreachable but silence compiler
}

DisplayType displayTypeFromString(const std::string& s) {
    if (s == "static_display")        return DisplayType::StaticDisplay;
    if (s == "warbird_heritage")      return DisplayType::WarbirdHeritage;
    if (s == "taxi_only")             return DisplayType::TaxiOnly;
    if (s == "military_static")       return DisplayType::MilitaryStatic;
    if (s == "hot_ramp")              return DisplayType::HotRamp;
    if (s == "media_photo_platform")  return DisplayType::MediaPhotoPlatform;
    if (s == "ramp_show")             return DisplayType::RampShow;
    throw std::runtime_error("ProjectFile: unknown display_type string: " + s);
}

// ---------------------------------------------------------------------------
// UUID v4
// ---------------------------------------------------------------------------
std::string ProjectFile::generateUuid() {
    static std::mt19937 rng{std::random_device{}()};
    static std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    std::array<uint32_t, 4> d = { dist(rng), dist(rng), dist(rng), dist(rng) };

    // Set version 4: bits 12-15 of d[1] (upper half)
    // UUID layout: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    // d[0] = time_low (32 bits)
    // d[1] high16 = time_mid (16 bits), low16 = time_hi_and_version (16 bits, top 4 = 4)
    // d[2] high8  = clock_seq_hi_and_res (8 bits, top 2 bits = 10), next 8 = clock_seq_low
    // d[3] = node (48 bits, split as 16+32)

    uint32_t a = d[0];
    uint32_t b = (d[1] & 0xFFFF0FFFu) | 0x00004000u; // version 4
    uint32_t c = (d[2] & 0x3FFFFFFFu) | 0x80000000u; // variant bits 10xx
    uint32_t e = d[3];

    char buf[37];
    snprintf(buf, sizeof(buf),
             "%08x-%04x-%04x-%04x-%04x%08x",
             a,
             (b >> 16) & 0xFFFFu,
             b & 0xFFFFu,
             (c >> 16) & 0xFFFFu,
             c & 0xFFFFu,
             e);
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// UTC timestamp
// ---------------------------------------------------------------------------
std::string ProjectFile::currentUtcTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// Helper: serialize a single aircraft to JSON
// ---------------------------------------------------------------------------
static json serializeAircraft(const arld::core::PlacedAircraft& ac) {
    json acj = {
        {"placement_id",  ac.placementId},
        {"library_id",    ac.libraryId},
        {"display_name",  ac.displayName},
        {"center_x",      ac.centerX},
        {"center_y",      ac.centerY},
        {"rotation_deg",  ac.rotationDeg},
        {"wingspan_ft",   ac.wingspanFt},
        {"length_ft",     ac.lengthFt},
        {"display_type",  arld::core::displayTypeToString(ac.displayType)}
    };
    if (!ac.tailNumber.empty())    acj["tail_number"]   = ac.tailNumber;
    if (!ac.owner.empty())         acj["owner"]         = ac.owner;
    if (!ac.fuelType.empty())      acj["fuel_type"]     = ac.fuelType;
    if (ac.hasHazmat)              acj["has_hazmat"]    = true;
    if (!ac.gearExtended)          acj["gear_extended"] = false;
    if (!ac.arrivalTime.empty())   acj["arrival_time"]   = ac.arrivalTime;
    if (!ac.departureTime.empty()) acj["departure_time"] = ac.departureTime;
    // LabelMode (default DisplayName is omitted to save space)
    using LM = arld::core::PlacedAircraft::LabelMode;
    if (ac.labelMode == LM::TailNumber) acj["label_mode"] = "tail_number";
    else if (ac.labelMode == LM::Hidden) acj["label_mode"] = "hidden";
    return acj;
}

// Helper: serialize boundary to JSON
static json serializeBoundary(const arld::core::RampBoundaryData& boundary) {
    json vertices = json::array();
    for (const auto& [x, y] : boundary.vertices)
        vertices.push_back({{"x", x}, {"y", y}});
    return json{{"vertices", vertices}, {"closed", boundary.closed}};
}

// ---------------------------------------------------------------------------
// Save (schema version 2 or 3)
// ---------------------------------------------------------------------------
void ProjectFile::save(const std::string& path, const ProjectData& data) {
    json j;
    // Use schema_version 3 when versions are non-empty; otherwise keep 2 for back-compat.
    const int sv = data.versions.empty() ? 2 : 3;
    j["arld_version"]  = "2.0.0";
    j["schema_version"] = sv;

    json meta = {
        {"title",        data.metadata.title},
        {"created_utc",  data.metadata.createdUtc.empty()
                             ? currentUtcTimestamp()
                             : data.metadata.createdUtc},
        {"modified_utc", currentUtcTimestamp()}
    };
    // Optional show metadata — only write when non-empty.
    if (!data.metadata.showDate.empty())  meta["show_date"]  = data.metadata.showDate;
    if (!data.metadata.showVenue.empty()) meta["show_venue"] = data.metadata.showVenue;
    j["metadata"] = meta;

    // Boundary
    j["ramp_boundary"] = serializeBoundary(data.boundary);

    // Aircraft
    json aircraft = json::array();
    for (const auto& ac : data.aircraft)
        aircraft.push_back(serializeAircraft(ac));
    j["aircraft"] = aircraft;

    // Overrides
    json overrides = json::array();
    for (const auto& ov : data.overrides) {
        overrides.push_back({
            {"placement_id_a",  ov.placementIdA},
            {"placement_id_b",  ov.placementIdB},
            {"justification",   ov.justification},
            {"username",        ov.username},
            {"timestamp_utc",   ov.timestampUtc}
        });
    }
    j["overrides"] = overrides;

    // Named layout versions (schema_version 3, Sprint 2-2)
    if (!data.versions.empty()) {
        json versions = json::array();
        for (const auto& ver : data.versions) {
            json vj;
            vj["id"]          = ver.id;
            vj["name"]        = ver.name;
            vj["created_utc"] = ver.createdUtc;
            vj["ramp_boundary"] = serializeBoundary(ver.boundary);
            json vacList = json::array();
            for (const auto& ac : ver.aircraft)
                vacList.push_back(serializeAircraft(ac));
            vj["aircraft"] = vacList;
            versions.push_back(std::move(vj));
        }
        j["versions"] = versions;
    }

    // Satellite image (optional — omit when empty)
    if (!data.satelliteImagePath.empty()) {
        j["satellite_image"] = {
            {"path",            data.satelliteImagePath},
            {"gsd_feet_per_px", data.satelliteGsdFeetPerPixel}
        };
    }

    // Clearance ruleset (omit when it is the FAA CoW default to save space)
    const auto defaultRules = arld::core::ClearanceRuleSet::faaCoW();
    if (data.clearanceRules.rulesetId != defaultRules.rulesetId) {
        j["clearance_ruleset"] = {
            {"ruleset_id",                  data.clearanceRules.rulesetId},
            {"display_name",                data.clearanceRules.displayName},
            {"static_display_wingtip_ft",   data.clearanceRules.staticDisplayWingtipFt},
            {"warbird_prop_arc_bonus_ft",    data.clearanceRules.warbirdPropArcBonusFt},
            {"taxi_only_corridor_ft",        data.clearanceRules.taxiOnlyCorridorFt},
            {"military_static_standoff_ft",  data.clearanceRules.militaryStaticStandoffFt},
            {"hot_ramp_standoff_ft",         data.clearanceRules.hotRampStandoffFt},
            {"media_photo_platform_ft",      data.clearanceRules.mediaPhotoPlatformFt},
            {"ramp_show_crowd_line_ft",      data.clearanceRules.rampShowCrowdLineFt}
        };
    }

    std::ofstream ofs(path);
    if (!ofs.is_open())
        throw std::runtime_error("ProjectFile::save: cannot open file: " + path);
    ofs << j.dump(2);
    if (!ofs.good())
        throw std::runtime_error("ProjectFile::save: write error: " + path);
}

// ---------------------------------------------------------------------------
// Pre-parse depth check: reject files with JSON nesting > 32 (Sprint 2-3-10)
// ---------------------------------------------------------------------------
static int jsonMaxDepth(const std::string& s) {
    int depth = 0, maxDepth = 0;
    bool inString = false;
    char prev = 0;
    for (char c : s) {
        if (c == '"' && prev != '\\') { inString = !inString; }
        if (!inString) {
            if (c == '{' || c == '[') maxDepth = std::max(maxDepth, ++depth);
            else if (c == '}' || c == ']') --depth;
        }
        prev = c;
    }
    return maxDepth;
}

// Helper: deserialize LabelMode from JSON value (Sprint 2-3-4)
static arld::core::PlacedAircraft::LabelMode labelModeFromString(const std::string& s) {
    using LM = arld::core::PlacedAircraft::LabelMode;
    if (s == "tail_number") return LM::TailNumber;
    if (s == "hidden")      return LM::Hidden;
    return LM::DisplayName; // default / "display_name"
}

// Helper: deserialize a single aircraft from JSON (used in both load paths)
static arld::core::PlacedAircraft deserializeAircraft(const json& ac) {
    arld::core::PlacedAircraft pa;
    pa.placementId   = ac.value("placement_id", "");
    pa.libraryId     = ac.value("library_id",   "");
    pa.displayName   = ac.value("display_name", "");
    pa.centerX       = ac.value("center_x",     0.0f);
    pa.centerY       = ac.value("center_y",     0.0f);
    pa.rotationDeg   = ac.value("rotation_deg", 0.0f);
    pa.wingspanFt    = ac.value("wingspan_ft",  0.0f);
    pa.lengthFt      = ac.value("length_ft",    0.0f);
    pa.displayType   = arld::core::displayTypeFromString(
        ac.value("display_type", std::string("static_display")));
    pa.tailNumber    = ac.value("tail_number",   std::string(""));
    pa.owner         = ac.value("owner",         std::string(""));
    pa.fuelType      = ac.value("fuel_type",     std::string(""));
    pa.hasHazmat     = ac.value("has_hazmat",    false);
    pa.gearExtended  = ac.value("gear_extended", true);
    pa.arrivalTime   = ac.value("arrival_time",   std::string(""));
    pa.departureTime = ac.value("departure_time", std::string(""));
    pa.labelMode     = labelModeFromString(ac.value("label_mode", std::string("display_name")));
    return pa;
}

// ---------------------------------------------------------------------------
// Load (supports schema version 1 migration and version 2)
// ---------------------------------------------------------------------------
ProjectData ProjectFile::load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open())
        throw std::runtime_error("ProjectFile::load: cannot open file: " + path);

    const std::string content((std::istreambuf_iterator<char>(ifs)),
                               std::istreambuf_iterator<char>());
    if (jsonMaxDepth(content) > 32)
        throw std::runtime_error(
            "ProjectFile::load: JSON nesting depth exceeds limit (max 32)");

    json j;
    try {
        j = json::parse(content);
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("ProjectFile::load: JSON parse error: ") + e.what());
    }

    if (!j.contains("schema_version") || !j["schema_version"].is_number_integer())
        throw std::runtime_error("ProjectFile::load: missing or invalid schema_version");

    int sv = j.value("schema_version", 0);
    if (sv < 1 || sv > 3)
        throw std::runtime_error("ProjectFile::load: unsupported schema_version: "
                                 + std::to_string(sv));

    ProjectData data;
    data.schemaVersion = sv;
    data.arldVersion   = j.value("arld_version", "0.0.0");

    // Metadata
    if (j.contains("metadata") && j["metadata"].is_object()) {
        const auto& meta = j["metadata"];
        data.metadata.title       = meta.value("title",        "Untitled Layout");
        data.metadata.showDate    = meta.value("show_date",    std::string(""));
        data.metadata.showVenue   = meta.value("show_venue",   std::string(""));
        data.metadata.createdUtc  = meta.value("created_utc",  "");
        data.metadata.modifiedUtc = meta.value("modified_utc", "");
    }

    // Boundary
    if (j.contains("ramp_boundary") && j["ramp_boundary"].is_object()) {
        const auto& rb = j["ramp_boundary"];
        data.boundary.closed = rb.value("closed", false);
        if (rb.contains("vertices") && rb["vertices"].is_array()) {
            for (const auto& v : rb["vertices"]) {
                float x = v.value("x", 0.0f);
                float y = v.value("y", 0.0f);
                data.boundary.vertices.emplace_back(x, y);
            }
        }
    }

    // Aircraft
    if (j.contains("aircraft") && j["aircraft"].is_array()) {
        for (const auto& ac : j["aircraft"]) {
            data.aircraft.push_back(deserializeAircraft(ac));
        }
    }

    // Overrides (v2 only; empty for v1 migration)
    if (j.contains("overrides") && j["overrides"].is_array()) {
        for (const auto& ov : j["overrides"]) {
            ClearanceOverride co;
            co.placementIdA  = ov.value("placement_id_a", "");
            co.placementIdB  = ov.value("placement_id_b", "");
            co.justification = ov.value("justification",  "");
            co.username      = ov.value("username",       "");
            co.timestampUtc  = ov.value("timestamp_utc",  "");
            data.overrides.push_back(std::move(co));
        }
    }

    // Named layout versions (sv=3 only; empty for sv=1/2)
    if (sv >= 3 && j.contains("versions") && j["versions"].is_array()) {
        for (const auto& vj : j["versions"]) {
            LayoutVersion ver;
            ver.id          = vj.value("id",          "");
            ver.name        = vj.value("name",         "");
            ver.createdUtc  = vj.value("created_utc",  "");

            // Boundary
            if (vj.contains("ramp_boundary") && vj["ramp_boundary"].is_object()) {
                const auto& rb = vj["ramp_boundary"];
                ver.boundary.closed = rb.value("closed", false);
                if (rb.contains("vertices") && rb["vertices"].is_array()) {
                    for (const auto& v : rb["vertices"])
                        ver.boundary.vertices.emplace_back(v.value("x", 0.0f), v.value("y", 0.0f));
                }
            }

            // Aircraft
            if (vj.contains("aircraft") && vj["aircraft"].is_array()) {
                for (const auto& ac : vj["aircraft"])
                    ver.aircraft.push_back(deserializeAircraft(ac));
            }

            data.versions.push_back(std::move(ver));
        }
    }

    // Satellite image (optional, Sprint 3-2-3)
    if (j.contains("satellite_image") && j["satellite_image"].is_object()) {
        const auto& si = j["satellite_image"];
        data.satelliteImagePath       = si.value("path",            std::string(""));
        data.satelliteGsdFeetPerPixel = si.value("gsd_feet_per_px", 0.0);
    }

    // Clearance ruleset (optional, Sprint 3-2-9)
    if (j.contains("clearance_ruleset") && j["clearance_ruleset"].is_object()) {
        const auto& cr = j["clearance_ruleset"];
        data.clearanceRules.rulesetId                = cr.value("ruleset_id",                   std::string("faa_cow"));
        data.clearanceRules.displayName              = cr.value("display_name",                 std::string("FAA Certificate of Waiver"));
        data.clearanceRules.staticDisplayWingtipFt   = cr.value("static_display_wingtip_ft",    25.0f);
        data.clearanceRules.warbirdPropArcBonusFt    = cr.value("warbird_prop_arc_bonus_ft",     35.0f);
        data.clearanceRules.taxiOnlyCorridorFt       = cr.value("taxi_only_corridor_ft",         50.0f);
        data.clearanceRules.militaryStaticStandoffFt = cr.value("military_static_standoff_ft",   50.0f);
        data.clearanceRules.hotRampStandoffFt        = cr.value("hot_ramp_standoff_ft",         100.0f);
        data.clearanceRules.mediaPhotoPlatformFt     = cr.value("media_photo_platform_ft",       15.0f);
        data.clearanceRules.rampShowCrowdLineFt      = cr.value("ramp_show_crowd_line_ft",      200.0f);
    }

    return data;
}

} // namespace arld::core
