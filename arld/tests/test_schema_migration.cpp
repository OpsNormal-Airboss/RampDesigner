#include <catch2/catch_test_macros.hpp>
#include <arld/core/ProjectFile.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

TEST_CASE("Schema migration: v1 file loads as v2 data", "[migration]") {
    // Build a minimal v1 JSON string (the old schema_version=1 format)
    const std::string v1json = R"({
        "arld_version": "0.5.0",
        "schema_version": 1,
        "metadata": {"title":"Test","created_utc":"2026-01-01T00:00:00Z","modified_utc":"2026-01-01T00:00:00Z"},
        "ramp_boundary": {"vertices":[],"closed":false},
        "aircraft": []
    })";

    // Write to temp file
    const std::string tmp = (fs::temp_directory_path() / "arld_migration_test.arld").string();
    {
        std::ofstream f(tmp);
        REQUIRE(f.is_open());
        f << v1json;
    }

    // Load it — should not throw
    arld::core::ProjectData data;
    REQUIRE_NOTHROW(data = arld::core::ProjectFile::load(tmp));

    // Verify migration defaults
    CHECK(data.overrides.empty());
    CHECK(data.aircraft.empty());
    CHECK(data.metadata.title == "Test");

    // Re-save and verify it loads fine as v2
    arld::core::ProjectFile::save(tmp, data);
    arld::core::ProjectData data2;
    REQUIRE_NOTHROW(data2 = arld::core::ProjectFile::load(tmp));
    CHECK(data2.overrides.empty());
    CHECK(data2.aircraft.empty());

    fs::remove(tmp);
}

TEST_CASE("Schema migration: v1 file with aircraft loads metadata defaults", "[migration]") {
    // A v1 file with one aircraft — the per-aircraft metadata fields should default
    const std::string v1json = R"({
        "arld_version": "0.5.0",
        "schema_version": 1,
        "metadata": {"title":"V1 Layout","created_utc":"2026-01-01T00:00:00Z","modified_utc":"2026-01-01T00:00:00Z"},
        "ramp_boundary": {"vertices":[],"closed":false},
        "aircraft": [
            {
                "placement_id": "00000000-0000-4000-8000-000000000001",
                "library_id": "north-american-p51-d",
                "display_name": "P-51D Mustang",
                "center_x": 100.0, "center_y": 200.0,
                "rotation_deg": 0.0,
                "wingspan_ft": 37.0, "length_ft": 32.0,
                "display_type": "warbird_heritage"
            }
        ]
    })";

    const std::string tmp = (fs::temp_directory_path() / "arld_migration_aircraft.arld").string();
    {
        std::ofstream f(tmp);
        REQUIRE(f.is_open());
        f << v1json;
    }

    arld::core::ProjectData data;
    REQUIRE_NOTHROW(data = arld::core::ProjectFile::load(tmp));

    REQUIRE(data.aircraft.size() == 1u);
    const auto& ac = data.aircraft[0];
    CHECK(ac.displayType == arld::core::DisplayType::WarbirdHeritage);
    // Per-aircraft metadata fields should have defaults (empty string / false)
    CHECK(ac.tailNumber.empty());
    CHECK(ac.owner.empty());
    CHECK(ac.fuelType.empty());
    CHECK(ac.hasHazmat == false);

    fs::remove(tmp);
}

TEST_CASE("Schema migration: schema_version > 2 throws", "[migration]") {
    const std::string json = R"({"schema_version": 99, "arld_version": "99.0.0"})";
    const std::string tmp = (fs::temp_directory_path() / "arld_migration_bad.arld").string();
    {
        std::ofstream f(tmp);
        f << json;
    }
    REQUIRE_THROWS(arld::core::ProjectFile::load(tmp));
    fs::remove(tmp);
}

TEST_CASE("Schema migration: schema_version 0 throws", "[migration]") {
    const std::string json = R"({"schema_version": 0, "arld_version": "0.0.0"})";
    const std::string tmp = (fs::temp_directory_path() / "arld_migration_zero.arld").string();
    {
        std::ofstream f(tmp);
        f << json;
    }
    REQUIRE_THROWS(arld::core::ProjectFile::load(tmp));
    fs::remove(tmp);
}

TEST_CASE("ProjectFile v2: overrides round-trip", "[migration]") {
    using namespace arld::core;

    ProjectData orig;
    orig.metadata.createdUtc  = "2026-05-11T00:00:00Z";
    orig.metadata.modifiedUtc = "2026-05-11T00:00:00Z";

    ClearanceOverride ov;
    ov.placementIdA  = ProjectFile::generateUuid();
    ov.placementIdB  = ProjectFile::generateUuid();
    ov.justification = "Aircraft shown statically in controlled environment with FAA approval";
    ov.username      = "testuser";
    ov.timestampUtc  = "2026-05-11T12:00:00Z";
    orig.overrides.push_back(ov);

    const std::string tmp = (fs::temp_directory_path() / "arld_migration_overrides.arld").string();
    REQUIRE_NOTHROW(ProjectFile::save(tmp, orig));

    ProjectData loaded;
    REQUIRE_NOTHROW(loaded = ProjectFile::load(tmp));

    REQUIRE(loaded.overrides.size() == 1u);
    CHECK(loaded.overrides[0].placementIdA  == ov.placementIdA);
    CHECK(loaded.overrides[0].placementIdB  == ov.placementIdB);
    CHECK(loaded.overrides[0].justification == ov.justification);
    CHECK(loaded.overrides[0].username      == ov.username);
    CHECK(loaded.overrides[0].timestampUtc  == ov.timestampUtc);

    fs::remove(tmp);
}

TEST_CASE("ProjectFile v2: per-aircraft metadata round-trip", "[migration]") {
    using namespace arld::core;

    ProjectData orig;
    orig.metadata.createdUtc  = "2026-05-11T00:00:00Z";
    orig.metadata.modifiedUtc = "2026-05-11T00:00:00Z";

    PlacedAircraft ac;
    ac.placementId = ProjectFile::generateUuid();
    ac.libraryId   = "north-american-p51-d";
    ac.displayName = "P-51D Mustang";
    ac.centerX     = 100.0f;
    ac.centerY     = 200.0f;
    ac.rotationDeg = 45.0f;
    ac.wingspanFt  = 37.0f;
    ac.lengthFt    = 32.0f;
    ac.displayType = DisplayType::WarbirdHeritage;
    ac.tailNumber  = "NX71JB";
    ac.owner       = "EAA AirVenture";
    ac.fuelType    = "100LL";
    ac.hasHazmat   = true;
    orig.aircraft.push_back(ac);

    const std::string tmp = (fs::temp_directory_path() / "arld_v2_metadata.arld").string();
    REQUIRE_NOTHROW(ProjectFile::save(tmp, orig));

    ProjectData loaded;
    REQUIRE_NOTHROW(loaded = ProjectFile::load(tmp));

    REQUIRE(loaded.aircraft.size() == 1u);
    const auto& lac = loaded.aircraft[0];
    CHECK(lac.tailNumber == "NX71JB");
    CHECK(lac.owner      == "EAA AirVenture");
    CHECK(lac.fuelType   == "100LL");
    CHECK(lac.hasHazmat  == true);

    fs::remove(tmp);
}
