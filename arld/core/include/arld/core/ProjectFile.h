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
};

struct RampBoundaryData {
    std::vector<std::pair<float, float>> vertices;   // (x, y) in scene feet
    bool closed = false;
};

struct ProjectMetadata {
    std::string title        = "Untitled Layout";
    std::string createdUtc;
    std::string modifiedUtc;
};

struct ProjectData {
    std::string      arldVersion   = "0.5.0";
    int              schemaVersion = 1;
    ProjectMetadata  metadata;
    RampBoundaryData boundary;
    std::vector<PlacedAircraft> aircraft;
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
