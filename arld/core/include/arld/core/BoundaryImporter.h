#pragma once
#include <arld/core/ProjectFile.h>
#include <string>

namespace arld::core {

/// Imports a ramp boundary polygon from KML or GeoJSON files.
/// Converts geographic coordinates to scene-ft using a flat-earth projection:
///   1 degree latitude  ≈ 364,566 ft
///   1 degree longitude ≈ 364,566 * cos(lat_radians) ft
/// The polygon centroid is mapped to scene origin (0, 0).
class BoundaryImporter {
public:
    /// Load a KML or GeoJSON file and return the first polygon as a RampBoundaryData.
    /// Detects format by file extension (.kml or .geojson/.json) or by content inspection.
    /// Throws std::runtime_error on parse failure.
    static RampBoundaryData importFile(const std::string& path);

    /// Parse KML content string.
    static RampBoundaryData importKml(const std::string& kmlContent);

    /// Parse GeoJSON content string.
    static RampBoundaryData importGeoJson(const std::string& geoJsonContent);
};

} // namespace arld::core
