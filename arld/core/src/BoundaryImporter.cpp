#include <arld/core/BoundaryImporter.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace arld::core {

namespace {

// Flat-earth conversion constants
static constexpr double kFtPerDegLat = 364566.0;

static RampBoundaryData projectToScene(const std::vector<std::pair<double,double>>& lonLats) {
    if (lonLats.size() < 3)
        throw std::runtime_error("BoundaryImporter: polygon must have at least 3 vertices");

    // Compute centroid
    double sumLon = 0.0, sumLat = 0.0;
    for (const auto& [lon, lat] : lonLats) {
        sumLon += lon;
        sumLat += lat;
    }
    const double n = static_cast<double>(lonLats.size());
    const double centLon = sumLon / n;
    const double centLat = sumLat / n;
    const double cosLat  = std::cos(centLat * M_PI / 180.0);

    RampBoundaryData boundary;
    boundary.closed = true;
    for (const auto& [lon, lat] : lonLats) {
        const double dx = (lon - centLon) * cosLat * kFtPerDegLat;
        const double dy = (lat - centLat) * kFtPerDegLat;
        // Flip Y so screen-down = south
        boundary.vertices.emplace_back(static_cast<float>(dx), static_cast<float>(-dy));
    }
    return boundary;
}

// Minimal KML <coordinates> parser.
// Expects whitespace-separated "lon,lat,alt" or "lon,lat" tokens.
static std::vector<std::pair<double,double>> parseKmlCoordinates(const std::string& coordStr) {
    std::vector<std::pair<double,double>> pts;
    std::istringstream iss(coordStr);
    std::string token;
    while (iss >> token) {
        if (token.empty()) continue;
        double lon = 0.0, lat = 0.0, alt = 0.0;
        // sscanf accepts "lon,lat,alt" or "lon,lat"
        int parsed = std::sscanf(token.c_str(), "%lf,%lf,%lf", &lon, &lat, &alt);
        if (parsed >= 2)
            pts.emplace_back(lon, lat);
    }
    return pts;
}

// Extract text between first occurrence of <tag> and </tag>.
static std::string extractXmlTag(const std::string& xml, const std::string& tag) {
    const std::string open  = "<" + tag + ">";
    const std::string close = "</" + tag + ">";
    const auto start = xml.find(open);
    if (start == std::string::npos) return "";
    const auto end = xml.find(close, start + open.size());
    if (end == std::string::npos) return "";
    return xml.substr(start + open.size(), end - start - open.size());
}

} // anonymous namespace

RampBoundaryData BoundaryImporter::importFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open())
        throw std::runtime_error("BoundaryImporter: cannot open file: " + path);

    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    // Detect by extension
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".kml")
        return importKml(content);

    if ((lower.size() >= 8 && lower.substr(lower.size() - 8) == ".geojson") ||
        (lower.size() >= 5 && lower.substr(lower.size() - 5) == ".json"))
        return importGeoJson(content);

    // Fall back to content inspection
    const auto trimmed = content.substr(0, std::min<size_t>(content.size(), 200));
    if (trimmed.find('<') != std::string::npos)
        return importKml(content);
    return importGeoJson(content);
}

RampBoundaryData BoundaryImporter::importKml(const std::string& kmlContent) {
    // Find <coordinates> element
    const std::string coordStr = extractXmlTag(kmlContent, "coordinates");
    if (coordStr.empty())
        throw std::runtime_error("BoundaryImporter: no <coordinates> element found in KML");

    auto pts = parseKmlCoordinates(coordStr);
    if (pts.empty())
        throw std::runtime_error("BoundaryImporter: no valid coordinate pairs found in KML");

    // KML polygons repeat the first vertex at the end — remove if present
    if (pts.size() > 3) {
        const auto& first = pts.front();
        const auto& last  = pts.back();
        if (std::abs(first.first - last.first) < 1e-9 &&
            std::abs(first.second - last.second) < 1e-9)
            pts.pop_back();
    }

    return projectToScene(pts);
}

RampBoundaryData BoundaryImporter::importGeoJson(const std::string& geoJsonContent) {
    json j;
    try {
        j = json::parse(geoJsonContent);
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("BoundaryImporter: JSON parse error: ") + e.what());
    }

    // Accept FeatureCollection, Feature, or bare Geometry
    // Walk the JSON tree to find the first Polygon geometry.
    std::function<const json*(const json&)> findPolygon = [&](const json& node) -> const json* {
        if (!node.is_object()) return nullptr;

        if (node.contains("type") && node["type"].is_string()) {
            const std::string t = node["type"].get<std::string>();
            if (t == "Polygon" && node.contains("coordinates"))
                return &node;

            if (t == "FeatureCollection" && node.contains("features") && node["features"].is_array()) {
                for (const auto& feat : node["features"]) {
                    if (const auto* g = findPolygon(feat)) return g;
                }
            }

            if (t == "Feature" && node.contains("geometry")) {
                return findPolygon(node["geometry"]);
            }

            if (t == "GeometryCollection" && node.contains("geometries") && node["geometries"].is_array()) {
                for (const auto& geom : node["geometries"]) {
                    if (const auto* g = findPolygon(geom)) return g;
                }
            }
        }
        return nullptr;
    };

    const json* polygonNode = findPolygon(j);
    if (!polygonNode)
        throw std::runtime_error("BoundaryImporter: no Polygon geometry found in GeoJSON");

    const auto& coords = (*polygonNode)["coordinates"];
    if (!coords.is_array() || coords.empty() || !coords[0].is_array())
        throw std::runtime_error("BoundaryImporter: invalid Polygon coordinates in GeoJSON");

    // Take the outer ring (index 0)
    const auto& ring = coords[0];
    std::vector<std::pair<double,double>> pts;
    pts.reserve(ring.size());
    for (const auto& pt : ring) {
        if (!pt.is_array() || pt.size() < 2)
            continue;
        double lon = pt[0].get<double>();
        double lat = pt[1].get<double>();
        pts.emplace_back(lon, lat);
    }

    if (pts.empty())
        throw std::runtime_error("BoundaryImporter: empty ring in GeoJSON Polygon");

    // GeoJSON polygons repeat first vertex — remove if present
    if (pts.size() > 3) {
        const auto& first = pts.front();
        const auto& last  = pts.back();
        if (std::abs(first.first - last.first) < 1e-9 &&
            std::abs(first.second - last.second) < 1e-9)
            pts.pop_back();
    }

    return projectToScene(pts);
}

} // namespace arld::core
