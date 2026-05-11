#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <arld/core/ProjectFile.h>
#include <arld/export/SvgExporter.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helper: read entire file to string
// ---------------------------------------------------------------------------
static std::string readFile(const std::string& path) {
    std::ifstream ifs(path);
    REQUIRE(ifs.is_open());
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

// ---------------------------------------------------------------------------
// UUID v4 format regex
// ---------------------------------------------------------------------------
static bool isUuidV4(const std::string& s) {
    static const std::regex kUuidRe(
        "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$");
    return std::regex_match(s, kUuidRe);
}

// ---------------------------------------------------------------------------
// TEST CASES
// ---------------------------------------------------------------------------

TEST_CASE("ProjectFile: generateUuid produces valid UUID v4 format", "[project_file]") {
    // Generate several and verify they all match the pattern and are unique.
    std::vector<std::string> uuids;
    for (int i = 0; i < 20; ++i) {
        const auto u = arld::core::ProjectFile::generateUuid();
        REQUIRE(isUuidV4(u));
        uuids.push_back(u);
    }
    // All should be unique
    std::sort(uuids.begin(), uuids.end());
    uuids.erase(std::unique(uuids.begin(), uuids.end()), uuids.end());
    REQUIRE(uuids.size() == 20u);
}

TEST_CASE("ProjectFile: save and load round-trip preserves all data — 15 aircraft", "[project_file]") {
    using namespace arld::core;

    // Build a ProjectData with 15 aircraft covering all display types + varied positions.
    ProjectData orig;
    orig.arldVersion              = "0.5.0";
    orig.schemaVersion            = 1;
    orig.metadata.title           = "Round-Trip Test Layout";
    orig.metadata.createdUtc      = "2026-05-10T10:00:00Z";
    orig.metadata.modifiedUtc     = "2026-05-10T10:00:00Z";

    // Boundary with 6 vertices, closed.
    orig.boundary.vertices = {
        {0.0f, 0.0f}, {500.0f, 0.0f}, {600.0f, 200.0f},
        {500.0f, 400.0f}, {100.0f, 400.0f}, {-50.0f, 200.0f}
    };
    orig.boundary.closed = true;

    const std::vector<DisplayType> kTypes = {
        DisplayType::StaticDisplay,
        DisplayType::WarbirdHeritage,
        DisplayType::TaxiOnly,
        DisplayType::MilitaryStatic,
        DisplayType::HotRamp,
        DisplayType::MediaPhotoPlatform,
        DisplayType::RampShow,
        DisplayType::StaticDisplay,
        DisplayType::WarbirdHeritage,
        DisplayType::TaxiOnly,
        DisplayType::MilitaryStatic,
        DisplayType::HotRamp,
        DisplayType::MediaPhotoPlatform,
        DisplayType::RampShow,
        DisplayType::StaticDisplay,
    };

    for (int i = 0; i < 15; ++i) {
        PlacedAircraft pa;
        pa.placementId  = ProjectFile::generateUuid();
        pa.libraryId    = "test-aircraft-" + std::to_string(i);
        pa.displayName  = "Aircraft " + std::to_string(i);
        pa.centerX      = static_cast<float>(i * 30 + 50);
        pa.centerY      = static_cast<float>(i * 15 + 20);
        pa.rotationDeg  = static_cast<float>(i * 23.7f);
        pa.wingspanFt   = 40.0f + static_cast<float>(i) * 3.0f;
        pa.lengthFt     = 30.0f + static_cast<float>(i) * 2.0f;
        pa.displayType  = kTypes[i];
        orig.aircraft.push_back(std::move(pa));
    }

    REQUIRE(orig.aircraft.size() == 15u);

    // Write to a temp file.
    const std::string tmpPath = (fs::temp_directory_path() / "arld_test_roundtrip.arld").string();
    REQUIRE_NOTHROW(ProjectFile::save(tmpPath, orig));

    // Load it back.
    ProjectData loaded;
    REQUIRE_NOTHROW(loaded = ProjectFile::load(tmpPath));

    // Verify top-level fields.
    CHECK(loaded.arldVersion   == orig.arldVersion);
    CHECK(loaded.schemaVersion == 1);
    CHECK(loaded.metadata.title == orig.metadata.title);
    CHECK(loaded.metadata.createdUtc == orig.metadata.createdUtc);
    // modified_utc is updated on save, so just check it's non-empty.
    CHECK(!loaded.metadata.modifiedUtc.empty());

    // Verify boundary.
    REQUIRE(loaded.boundary.vertices.size() == 6u);
    CHECK(loaded.boundary.closed == true);
    for (size_t v = 0; v < orig.boundary.vertices.size(); ++v) {
        CHECK(loaded.boundary.vertices[v].first  == Catch::Approx(orig.boundary.vertices[v].first).margin(0.001f));
        CHECK(loaded.boundary.vertices[v].second == Catch::Approx(orig.boundary.vertices[v].second).margin(0.001f));
    }

    // Verify all 15 aircraft.
    REQUIRE(loaded.aircraft.size() == 15u);
    for (size_t i = 0; i < orig.aircraft.size(); ++i) {
        const auto& o = orig.aircraft[i];
        const auto& l = loaded.aircraft[i];
        CHECK(l.placementId == o.placementId);
        CHECK(l.libraryId   == o.libraryId);
        CHECK(l.displayName == o.displayName);
        CHECK(l.centerX     == Catch::Approx(o.centerX).margin(0.001f));
        CHECK(l.centerY     == Catch::Approx(o.centerY).margin(0.001f));
        CHECK(l.rotationDeg == Catch::Approx(o.rotationDeg).margin(0.001f));
        CHECK(l.wingspanFt  == Catch::Approx(o.wingspanFt).margin(0.001f));
        CHECK(l.lengthFt    == Catch::Approx(o.lengthFt).margin(0.001f));
        CHECK(l.displayType == o.displayType);
    }

    fs::remove(tmpPath);
}

TEST_CASE("ProjectFile: load rejects wrong schema_version", "[project_file]") {
    const std::string tmpPath = (fs::temp_directory_path() / "arld_test_bad_schema.arld").string();

    // Write a file with schema_version = 99.
    {
        std::ofstream ofs(tmpPath);
        ofs << R"({"arld_version":"0.5.0","schema_version":99,"metadata":{"title":"T","created_utc":"2026-05-10T00:00:00Z","modified_utc":"2026-05-10T00:00:00Z"},"ramp_boundary":{"vertices":[],"closed":false},"aircraft":[]})";
    }

    REQUIRE_THROWS_AS(arld::core::ProjectFile::load(tmpPath), std::runtime_error);
    fs::remove(tmpPath);
}

TEST_CASE("ProjectFile: load rejects invalid JSON", "[project_file]") {
    const std::string tmpPath = (fs::temp_directory_path() / "arld_test_bad_json.arld").string();

    {
        std::ofstream ofs(tmpPath);
        ofs << "{ this is not valid JSON !!! ]]]";
    }

    REQUIRE_THROWS_AS(arld::core::ProjectFile::load(tmpPath), std::runtime_error);
    fs::remove(tmpPath);
}

TEST_CASE("ProjectFile: boundary round-trip preserves vertices and closed state", "[project_file]") {
    using namespace arld::core;

    ProjectData orig;
    orig.metadata.createdUtc  = "2026-05-10T00:00:00Z";
    orig.metadata.modifiedUtc = "2026-05-10T00:00:00Z";
    orig.boundary.vertices = {
        {10.5f, 20.3f}, {300.0f, -5.0f}, {150.0f, 400.75f}
    };
    orig.boundary.closed = true;

    const std::string tmpPath = (fs::temp_directory_path() / "arld_test_boundary.arld").string();
    REQUIRE_NOTHROW(ProjectFile::save(tmpPath, orig));

    ProjectData loaded;
    REQUIRE_NOTHROW(loaded = ProjectFile::load(tmpPath));

    CHECK(loaded.boundary.closed == true);
    REQUIRE(loaded.boundary.vertices.size() == 3u);
    CHECK(loaded.boundary.vertices[0].first  == Catch::Approx(10.5f).margin(0.001f));
    CHECK(loaded.boundary.vertices[0].second == Catch::Approx(20.3f).margin(0.001f));
    CHECK(loaded.boundary.vertices[2].second == Catch::Approx(400.75f).margin(0.001f));

    // Open boundary
    ProjectData orig2;
    orig2.metadata.createdUtc  = "2026-05-10T00:00:00Z";
    orig2.metadata.modifiedUtc = "2026-05-10T00:00:00Z";
    orig2.boundary.vertices = {{0.0f, 0.0f}, {100.0f, 0.0f}};
    orig2.boundary.closed = false;

    REQUIRE_NOTHROW(ProjectFile::save(tmpPath, orig2));
    ProjectData loaded2;
    REQUIRE_NOTHROW(loaded2 = ProjectFile::load(tmpPath));
    CHECK(loaded2.boundary.closed == false);
    REQUIRE(loaded2.boundary.vertices.size() == 2u);

    fs::remove(tmpPath);
}

TEST_CASE("SvgExporter: exportLayout produces non-empty SVG file", "[svg_exporter]") {
    arld::core::ProjectData data;
    data.metadata.title = "Test Layout";
    data.metadata.createdUtc  = "2026-05-10T00:00:00Z";
    data.metadata.modifiedUtc = "2026-05-10T00:00:00Z";

    // One boundary vertex set.
    data.boundary.vertices = {{0.0f,0.0f},{400.0f,0.0f},{400.0f,300.0f},{0.0f,300.0f}};
    data.boundary.closed = true;

    // Two aircraft.
    arld::core::PlacedAircraft ac1;
    ac1.placementId = arld::core::ProjectFile::generateUuid();
    ac1.libraryId   = "p51-d";
    ac1.displayName = "P-51D Mustang";
    ac1.centerX     = 100.0f; ac1.centerY = 150.0f;
    ac1.rotationDeg = 45.0f;
    ac1.wingspanFt  = 37.0f; ac1.lengthFt = 32.0f;
    ac1.displayType = arld::core::DisplayType::WarbirdHeritage;
    data.aircraft.push_back(ac1);

    arld::core::PlacedAircraft ac2;
    ac2.placementId = arld::core::ProjectFile::generateUuid();
    ac2.libraryId   = "f-16";
    ac2.displayName = "F-16 Falcon";
    ac2.centerX     = 250.0f; ac2.centerY = 150.0f;
    ac2.rotationDeg = 0.0f;
    ac2.wingspanFt  = 32.0f; ac2.lengthFt = 49.0f;
    ac2.displayType = arld::core::DisplayType::MilitaryStatic;
    data.aircraft.push_back(ac2);

    const std::string tmpPath = (fs::temp_directory_path() / "arld_test_export.svg").string();
    arld::export_::SvgExporter exporter;
    REQUIRE_NOTHROW(exporter.exportLayout(data, tmpPath));

    const std::string content = readFile(tmpPath);
    CHECK(!content.empty());
    // Must start with XML declaration or <svg tag.
    CHECK((content.find("<?xml") != std::string::npos ||
           content.find("<svg") != std::string::npos));

    fs::remove(tmpPath);
}

TEST_CASE("SvgExporter: SVG output contains expected aircraft IDs", "[svg_exporter]") {
    arld::core::ProjectData data;
    data.metadata.title       = "ID Check";
    data.metadata.createdUtc  = "2026-05-10T00:00:00Z";
    data.metadata.modifiedUtc = "2026-05-10T00:00:00Z";

    const std::vector<std::string> names = {"Alpha Jet", "B-17 Flying Fortress", "C-130 Hercules"};
    float cx = 50.0f;
    for (const auto& name : names) {
        arld::core::PlacedAircraft ac;
        ac.placementId = arld::core::ProjectFile::generateUuid();
        ac.libraryId   = "test";
        ac.displayName = name;
        ac.centerX = cx; ac.centerY = 50.0f;
        ac.wingspanFt = 40.0f; ac.lengthFt = 30.0f;
        ac.rotationDeg = 0.0f;
        ac.displayType = arld::core::DisplayType::StaticDisplay;
        data.aircraft.push_back(std::move(ac));
        cx += 100.0f;
    }

    const std::string tmpPath = (fs::temp_directory_path() / "arld_test_ids.svg").string();
    arld::export_::SvgExporter exporter;
    REQUIRE_NOTHROW(exporter.exportLayout(data, tmpPath));

    const std::string content = readFile(tmpPath);
    for (const auto& name : names) {
        INFO("Expected display name in SVG: " << name);
        CHECK(content.find(name) != std::string::npos);
    }

    fs::remove(tmpPath);
}

TEST_CASE("SvgExporter: empty layout produces valid SVG", "[svg_exporter]") {
    arld::core::ProjectData data;
    data.metadata.title       = "Empty";
    data.metadata.createdUtc  = "2026-05-10T00:00:00Z";
    data.metadata.modifiedUtc = "2026-05-10T00:00:00Z";
    // No boundary, no aircraft.

    const std::string tmpPath = (fs::temp_directory_path() / "arld_test_empty.svg").string();
    arld::export_::SvgExporter exporter;
    REQUIRE_NOTHROW(exporter.exportLayout(data, tmpPath));

    const std::string content = readFile(tmpPath);
    CHECK(!content.empty());
    CHECK(content.find("<svg") != std::string::npos);
    CHECK(content.find("</svg>") != std::string::npos);
    // Title text should appear.
    CHECK(content.find("Empty") != std::string::npos);

    fs::remove(tmpPath);
}
