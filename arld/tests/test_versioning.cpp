#include <catch2/catch_test_macros.hpp>
#include <arld/core/BoundaryImporter.h>
#include <arld/core/LayoutDiffer.h>
#include <arld/core/ProjectFile.h>
#include <arld/core/UndoStack.h>
#include <arld/core/ICommand.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace arld::core;

// ---------------------------------------------------------------------------
// Simple command for testing
// ---------------------------------------------------------------------------
namespace {
struct CountCmd : ICommand {
    int* counter;
    int  inc;
    std::string label;
    CountCmd(int* c, int i, std::string l)
        : counter(c), inc(i), label(std::move(l)) {}
    void execute() override { *counter += inc; }
    void undo()    override { *counter -= inc; }
    std::string describe() const override { return label; }
};
} // anonymous namespace

// ---------------------------------------------------------------------------
// LayoutVersion round-trip
// ---------------------------------------------------------------------------
TEST_CASE("LayoutVersion round-trips through ProjectFile save/load", "[versioning]") {
    ProjectData orig;
    orig.metadata.createdUtc  = "2026-05-11T00:00:00Z";
    orig.metadata.modifiedUtc = "2026-05-11T00:00:00Z";

    LayoutVersion ver;
    ver.id         = ProjectFile::generateUuid();
    ver.name       = "Version A";
    ver.createdUtc = "2026-05-11T12:00:00Z";
    ver.boundary.closed = true;
    ver.boundary.vertices.emplace_back(0.0f, 0.0f);
    ver.boundary.vertices.emplace_back(100.0f, 0.0f);
    ver.boundary.vertices.emplace_back(100.0f, 100.0f);

    PlacedAircraft ac;
    ac.placementId = ProjectFile::generateUuid();
    ac.libraryId   = "north-american-p51-d";
    ac.displayName = "P-51D Mustang";
    ac.centerX     = 50.0f;
    ac.centerY     = 50.0f;
    ac.wingspanFt  = 37.0f;
    ac.lengthFt    = 32.0f;
    ac.displayType = DisplayType::WarbirdHeritage;
    ver.aircraft.push_back(ac);
    orig.versions.push_back(ver);

    const std::string tmp = (fs::temp_directory_path() / "arld_versioning_test.arld").string();
    REQUIRE_NOTHROW(ProjectFile::save(tmp, orig));

    ProjectData loaded;
    REQUIRE_NOTHROW(loaded = ProjectFile::load(tmp));

    REQUIRE(loaded.versions.size() == 1u);
    const auto& lver = loaded.versions[0];
    CHECK(lver.id   == ver.id);
    CHECK(lver.name == "Version A");
    CHECK(lver.createdUtc == "2026-05-11T12:00:00Z");
    CHECK(lver.boundary.closed == true);
    REQUIRE(lver.boundary.vertices.size() == 3u);
    CHECK(lver.boundary.vertices[0].first  == 0.0f);
    CHECK(lver.boundary.vertices[1].first  == 100.0f);

    REQUIRE(lver.aircraft.size() == 1u);
    const auto& lac = lver.aircraft[0];
    CHECK(lac.libraryId  == "north-american-p51-d");
    CHECK(lac.centerX    == 50.0f);
    CHECK(lac.displayType == DisplayType::WarbirdHeritage);

    fs::remove(tmp);
}

TEST_CASE("ProjectFile save with versions writes schema_version 3", "[versioning]") {
    ProjectData data;
    data.metadata.createdUtc  = "2026-05-11T00:00:00Z";
    data.metadata.modifiedUtc = "2026-05-11T00:00:00Z";

    LayoutVersion ver;
    ver.id         = ProjectFile::generateUuid();
    ver.name       = "Test";
    ver.createdUtc = "2026-05-11T00:00:00Z";
    data.versions.push_back(ver);

    const std::string tmp = (fs::temp_directory_path() / "arld_sv3_test.arld").string();
    REQUIRE_NOTHROW(ProjectFile::save(tmp, data));

    // Read raw file to check schema_version
    std::ifstream ifs(tmp);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    CHECK(content.find("\"schema_version\": 3") != std::string::npos);
    ifs.close();

    // Also verify it loads cleanly
    ProjectData loaded;
    REQUIRE_NOTHROW(loaded = ProjectFile::load(tmp));
    CHECK(loaded.schemaVersion == 3);
    REQUIRE(loaded.versions.size() == 1u);
    CHECK(loaded.versions[0].name == "Test");

    fs::remove(tmp);
}

TEST_CASE("ProjectFile load v2 file gives empty versions list", "[versioning]") {
    const std::string v2json = R"({
        "arld_version": "1.1.0",
        "schema_version": 2,
        "metadata": {"title":"Test","created_utc":"2026-01-01T00:00:00Z","modified_utc":"2026-01-01T00:00:00Z"},
        "ramp_boundary": {"vertices":[],"closed":false},
        "aircraft": [],
        "overrides": []
    })";

    const std::string tmp = (fs::temp_directory_path() / "arld_v2_noversions.arld").string();
    {
        std::ofstream f(tmp);
        f << v2json;
    }

    ProjectData loaded;
    REQUIRE_NOTHROW(loaded = ProjectFile::load(tmp));
    CHECK(loaded.versions.empty());

    fs::remove(tmp);
}

// ---------------------------------------------------------------------------
// LayoutDiffer tests
// ---------------------------------------------------------------------------
TEST_CASE("LayoutDiffer: identifies added, removed, moved aircraft", "[delta]") {
    PlacedAircraft acBase;
    acBase.placementId = "id-base-1";
    acBase.centerX = 100.0f;
    acBase.centerY = 200.0f;

    PlacedAircraft acMoved;
    acMoved.placementId = "id-base-2";
    acMoved.centerX = 50.0f;
    acMoved.centerY = 50.0f;

    PlacedAircraft acMovedCompare;
    acMovedCompare.placementId = "id-base-2";
    acMovedCompare.centerX = 80.0f;  // moved
    acMovedCompare.centerY = 80.0f;

    PlacedAircraft acAdded;
    acAdded.placementId = "id-added-1";
    acAdded.centerX = 300.0f;
    acAdded.centerY = 300.0f;

    std::vector<PlacedAircraft> base    = { acBase, acMoved };
    std::vector<PlacedAircraft> compare = { acMovedCompare, acAdded };

    auto result = LayoutDiffer::diff(base, compare);

    // Find each by id
    const AircraftDelta* dMoved   = nullptr;
    const AircraftDelta* dAdded   = nullptr;
    const AircraftDelta* dRemoved = nullptr;
    for (const auto& d : result.deltas) {
        if (d.aircraft.placementId == "id-base-2")  dMoved   = &d;
        if (d.aircraft.placementId == "id-added-1") dAdded   = &d;
        if (d.aircraft.placementId == "id-base-1")  dRemoved = &d;
    }

    REQUIRE(dMoved   != nullptr);
    REQUIRE(dAdded   != nullptr);
    REQUIRE(dRemoved != nullptr);

    CHECK(dMoved->change   == DeltaChange::Moved);
    CHECK(dAdded->change   == DeltaChange::Added);
    CHECK(dRemoved->change == DeltaChange::Removed);

    // Moved: fromX/fromY should be the base position
    CHECK(dMoved->fromX == 50.0f);
    CHECK(dMoved->fromY == 50.0f);
}

TEST_CASE("LayoutDiffer: unchanged aircraft with same id and position", "[delta]") {
    PlacedAircraft ac;
    ac.placementId = "same-id";
    ac.centerX = 100.0f;
    ac.centerY = 200.0f;

    // Identical in both versions
    auto result = LayoutDiffer::diff({ac}, {ac});

    REQUIRE(result.deltas.size() == 1u);
    CHECK(result.deltas[0].change == DeltaChange::Unchanged);
}

// ---------------------------------------------------------------------------
// BoundaryImporter tests
// ---------------------------------------------------------------------------
TEST_CASE("BoundaryImporter: parses GeoJSON Polygon to RampBoundaryData", "[kml]") {
    const std::string geojson = R"({
        "type": "FeatureCollection",
        "features": [{
            "type": "Feature",
            "geometry": {
                "type": "Polygon",
                "coordinates": [[
                    [-89.0, 44.5, 0],
                    [-89.001, 44.5, 0],
                    [-89.001, 44.501, 0],
                    [-89.0, 44.501, 0],
                    [-89.0, 44.5, 0]
                ]]
            },
            "properties": {}
        }]
    })";

    RampBoundaryData bd;
    REQUIRE_NOTHROW(bd = BoundaryImporter::importGeoJson(geojson));
    CHECK(bd.closed == true);
    // Closing vertex is removed, so 4 vertices
    CHECK(bd.vertices.size() == 4u);
    // Centroid is at origin so values should be small
    for (const auto& [x, y] : bd.vertices) {
        CHECK(std::abs(x) < 500.0f);
        CHECK(std::abs(y) < 500.0f);
    }
}

TEST_CASE("BoundaryImporter: parses KML coordinates to RampBoundaryData", "[kml]") {
    const std::string kml = R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Placemark>
    <Polygon>
      <outerBoundaryIs>
        <LinearRing>
          <coordinates>
            -89.000,44.500,0
            -89.001,44.500,0
            -89.001,44.501,0
            -89.000,44.501,0
            -89.000,44.500,0
          </coordinates>
        </LinearRing>
      </outerBoundaryIs>
    </Polygon>
  </Placemark>
</kml>)";

    RampBoundaryData bd;
    REQUIRE_NOTHROW(bd = BoundaryImporter::importKml(kml));
    CHECK(bd.closed == true);
    CHECK(bd.vertices.size() == 4u);
}

TEST_CASE("BoundaryImporter: throws on invalid JSON content", "[kml]") {
    const std::string bad = "this is not json {{{";
    REQUIRE_THROWS(BoundaryImporter::importGeoJson(bad));
}

// ---------------------------------------------------------------------------
// UndoStack::history and goToIndex
// ---------------------------------------------------------------------------
TEST_CASE("UndoStack::history returns commands in order", "[undo]") {
    UndoStack stack;
    int counter = 0;
    stack.push(std::make_unique<CountCmd>(&counter, 1, "cmd-A"));
    stack.push(std::make_unique<CountCmd>(&counter, 2, "cmd-B"));
    stack.push(std::make_unique<CountCmd>(&counter, 3, "cmd-C"));

    CHECK(counter == 6);

    const auto hist = stack.history();
    REQUIRE(hist.size() == 3u);
    CHECK(hist[0] == "cmd-A");
    CHECK(hist[1] == "cmd-B");
    CHECK(hist[2] == "cmd-C");

    CHECK(stack.currentIndex() == 2);
}

TEST_CASE("UndoStack::goToIndex undoes to target", "[undo]") {
    UndoStack stack;
    int counter = 0;
    stack.push(std::make_unique<CountCmd>(&counter, 1, "A"));
    stack.push(std::make_unique<CountCmd>(&counter, 2, "B"));
    stack.push(std::make_unique<CountCmd>(&counter, 4, "C"));

    // After 3 pushes: counter == 7, currentIndex == 2
    CHECK(counter == 7);
    CHECK(stack.currentIndex() == 2);

    // Go back to after index 0 (only cmd-A executed)
    stack.goToIndex(0);
    CHECK(counter == 1);
    CHECK(stack.currentIndex() == 0);

    // Redo to index 2
    stack.goToIndex(2);
    CHECK(counter == 7);
    CHECK(stack.currentIndex() == 2);

    // Go to -1 (nothing executed)
    stack.goToIndex(-1);
    CHECK(counter == 0);
    CHECK(stack.currentIndex() == -1);
}
