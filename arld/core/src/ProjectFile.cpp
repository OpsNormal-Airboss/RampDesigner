#include <arld/core/ProjectFile.h>
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
// Save (schema version 2)
// ---------------------------------------------------------------------------
void ProjectFile::save(const std::string& path, const ProjectData& data) {
    json j;
    j["arld_version"]  = "1.1.0";
    j["schema_version"] = 2;

    j["metadata"] = {
        {"title",        data.metadata.title},
        {"created_utc",  data.metadata.createdUtc.empty()
                             ? currentUtcTimestamp()
                             : data.metadata.createdUtc},
        {"modified_utc", currentUtcTimestamp()}
    };

    // Boundary
    json vertices = json::array();
    for (const auto& [x, y] : data.boundary.vertices)
        vertices.push_back({{"x", x}, {"y", y}});
    j["ramp_boundary"] = {
        {"vertices", vertices},
        {"closed",   data.boundary.closed}
    };

    // Aircraft
    json aircraft = json::array();
    for (const auto& ac : data.aircraft) {
        json acj = {
            {"placement_id",  ac.placementId},
            {"library_id",    ac.libraryId},
            {"display_name",  ac.displayName},
            {"center_x",      ac.centerX},
            {"center_y",      ac.centerY},
            {"rotation_deg",  ac.rotationDeg},
            {"wingspan_ft",   ac.wingspanFt},
            {"length_ft",     ac.lengthFt},
            {"display_type",  displayTypeToString(ac.displayType)}
        };
        // Per-aircraft metadata fields (omit empty strings to keep files clean)
        if (!ac.tailNumber.empty()) acj["tail_number"] = ac.tailNumber;
        if (!ac.owner.empty())      acj["owner"]       = ac.owner;
        if (!ac.fuelType.empty())   acj["fuel_type"]   = ac.fuelType;
        if (ac.hasHazmat)           acj["has_hazmat"]  = true;
        aircraft.push_back(std::move(acj));
    }
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

    std::ofstream ofs(path);
    if (!ofs.is_open())
        throw std::runtime_error("ProjectFile::save: cannot open file: " + path);
    ofs << j.dump(2);
    if (!ofs.good())
        throw std::runtime_error("ProjectFile::save: write error: " + path);
}

// ---------------------------------------------------------------------------
// Load (supports schema version 1 migration and version 2)
// ---------------------------------------------------------------------------
ProjectData ProjectFile::load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open())
        throw std::runtime_error("ProjectFile::load: cannot open file: " + path);

    json j;
    try {
        ifs >> j;
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("ProjectFile::load: JSON parse error: ") + e.what());
    }

    if (!j.contains("schema_version") || !j["schema_version"].is_number_integer())
        throw std::runtime_error("ProjectFile::load: missing or invalid schema_version");

    int sv = j.value("schema_version", 0);
    if (sv < 1 || sv > 2)
        throw std::runtime_error("ProjectFile::load: unsupported schema_version: "
                                 + std::to_string(sv));

    ProjectData data;
    data.schemaVersion = sv;
    data.arldVersion   = j.value("arld_version", "0.0.0");

    // Metadata
    if (j.contains("metadata") && j["metadata"].is_object()) {
        const auto& meta = j["metadata"];
        data.metadata.title       = meta.value("title",        "Untitled Layout");
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
            PlacedAircraft pa;
            pa.placementId  = ac.value("placement_id", "");
            pa.libraryId    = ac.value("library_id",   "");
            pa.displayName  = ac.value("display_name", "");
            pa.centerX      = ac.value("center_x",     0.0f);
            pa.centerY      = ac.value("center_y",     0.0f);
            pa.rotationDeg  = ac.value("rotation_deg", 0.0f);
            pa.wingspanFt   = ac.value("wingspan_ft",  0.0f);
            pa.lengthFt     = ac.value("length_ft",    0.0f);
            pa.displayType  = displayTypeFromString(
                ac.value("display_type", std::string("static_display")));
            // Per-aircraft metadata (v2; use defaults for v1 migration)
            pa.tailNumber   = ac.value("tail_number", std::string(""));
            pa.owner        = ac.value("owner",       std::string(""));
            pa.fuelType     = ac.value("fuel_type",   std::string(""));
            pa.hasHazmat    = ac.value("has_hazmat",  false);
            data.aircraft.push_back(std::move(pa));
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

    return data;
}

} // namespace arld::core
