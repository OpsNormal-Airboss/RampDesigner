#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace arld::core {

// ---------------------------------------------------------------------------
// Data structures for the .arld project file format
// ---------------------------------------------------------------------------

struct PlacedAircraft {
    std::string placementId;
    std::string libraryId;
    std::string displayName;
    float centerX      = 0.0f;
    float centerY      = 0.0f;
    float rotationDeg  = 0.0f;
    float wingspanFt   = 0.0f;
    float lengthFt     = 0.0f;
    DisplayType displayType = DisplayType::StaticDisplay;
    // Per-aircraft metadata (Sprint 1-2-5 / 1-2-7)
    std::string tailNumber;
    std::string owner;
    std::string fuelType;
    bool        hasHazmat    = false;
    // Gear state (Sprint 1-4-2); safe default = gear down
    bool        gearExtended = true;
};

struct RampBoundaryData {
    std::vector<std::pair<float, float>> vertices;   // (x, y) in scene feet
    bool closed = false;
};

struct ProjectMetadata {
    std::string title        = "Untitled Layout";
    std::string showDate;    // optional, e.g. "2026-07-04"
    std::string showVenue;   // optional, e.g. "EAA AirVenture, Oshkosh WI"
    std::string createdUtc;
    std::string modifiedUtc;
};

// Clearance override record (Sprint 1-2-6)
struct ClearanceOverride {
    std::string placementIdA;
    std::string placementIdB;
    std::string justification;   // min 20 chars
    std::string username;
    std::string timestampUtc;
};

// Named layout snapshot (Sprint 2-2)
struct LayoutVersion {
    std::string               id;           // UUID v4
    std::string               name;         // user-defined label, e.g. "Version A"
    std::string               createdUtc;
    RampBoundaryData          boundary;
    std::vector<PlacedAircraft> aircraft;
};

struct ProjectData {
    std::string      arldVersion   = "1.1.0";
    int              schemaVersion = 2;
    ProjectMetadata  metadata;
    RampBoundaryData boundary;
    std::vector<PlacedAircraft>    aircraft;
    std::vector<ClearanceOverride> overrides;
    std::vector<LayoutVersion>     versions;  // named snapshots (Sprint 2-2)
};

// ---------------------------------------------------------------------------
// ProjectFile — serialization / deserialization utilities
// ---------------------------------------------------------------------------
class ProjectFile {
public:
    /// Serialize @p data to UTF-8 JSON at @p path.
    /// Throws std::runtime_error on I/O or serialization failure.
    static void save(const std::string& path, const ProjectData& data);

    /// Deserialize the project file at @p path.
    /// Throws std::runtime_error on I/O, malformed JSON, or wrong schema_version.
    /// Supports schema_version 1 (migrated) and 2 (native).
    static ProjectData load(const std::string& path);

    /// Generate a RFC 4122 version-4 UUID string.
    static std::string generateUuid();

    /// Return the current UTC time as an ISO 8601 string, e.g. "2026-05-10T14:30:00Z".
    static std::string currentUtcTimestamp();
};

// ---------------------------------------------------------------------------
// Helpers: DisplayType ↔ snake_case string
// ---------------------------------------------------------------------------
/// Returns the snake_case project-file string for @p dt.
/// Never throws.
std::string displayTypeToString(DisplayType dt);

/// Parses a snake_case string to DisplayType.
/// Throws std::runtime_error on unknown strings.
DisplayType displayTypeFromString(const std::string& s);

} // namespace arld::core
