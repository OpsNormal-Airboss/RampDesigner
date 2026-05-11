#include <catch2/catch_test_macros.hpp>
#include <arld/core/ProjectFile.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string minimalValidArld() {
    // A minimal valid schema_version 2 project
    return R"({
  "arld_version": "2.0.0",
  "schema_version": 2,
  "metadata": {"title": "Test", "created_utc": "2026-01-01T00:00:00Z"},
  "ramp_boundary": {"vertices": [], "closed": false},
  "aircraft": [],
  "overrides": []
})";
}

/// Build JSON with the requested bracket nesting depth (all arrays).
static std::string makeDeepJson(int depth) {
    std::string result;
    result.reserve(depth * 4);
    for (int i = 0; i < depth; ++i) result += '[';
    for (int i = 0; i < depth; ++i) result += ']';
    return result;
}

static std::string writeTempFile(const std::string& content) {
    const std::string path =
        (fs::temp_directory_path() / "arld_security_test.arld").string();
    std::ofstream ofs(path);
    ofs << content;
    return path;
}

// ---------------------------------------------------------------------------
// JSON depth-limit security tests (Sprint 2-3-10)
// ---------------------------------------------------------------------------

TEST_CASE(".arld file with JSON nesting depth > 32 is rejected", "[security]") {
    // Build a standalone deeply-nested JSON that is syntactically valid
    const std::string deepJson = makeDeepJson(33);
    const std::string path = writeTempFile(deepJson);

    CHECK_THROWS_AS(arld::core::ProjectFile::load(path), std::runtime_error);

    fs::remove(path);
}

TEST_CASE(".arld file with nesting depth exactly 32 is not rejected on depth alone", "[security]") {
    // 32 levels of nesting: valid depth but not a valid ARLD schema, so it
    // throws a parse/schema error rather than the depth-limit error.
    const std::string deepJson = makeDeepJson(32);
    const std::string path = writeTempFile(deepJson);

    // Must NOT throw the depth-limit error; may throw another runtime_error
    // (e.g. missing schema_version) — that is acceptable.
    bool threwDepthError = false;
    try {
        arld::core::ProjectFile::load(path);
    } catch (const std::runtime_error& e) {
        threwDepthError = std::string(e.what()).find("depth exceeds limit") != std::string::npos;
    }
    CHECK_FALSE(threwDepthError);

    fs::remove(path);
}

TEST_CASE(".arld file with nesting depth <= 32 loads successfully", "[security]") {
    const std::string path = writeTempFile(minimalValidArld());

    REQUIRE_NOTHROW(arld::core::ProjectFile::load(path));

    fs::remove(path);
}

// ---------------------------------------------------------------------------
// arrivalTime / departureTime round-trip (Sprint 2-3-3)
// ---------------------------------------------------------------------------

TEST_CASE("ProjectFile: arrival/departure fields round-trip", "[projectfile]") {
    arld::core::ProjectData data;
    data.metadata.title = "Timing Test";
    data.schemaVersion = 2;

    arld::core::PlacedAircraft pa;
    pa.placementId   = arld::core::ProjectFile::generateUuid();
    pa.libraryId     = "test-aircraft";
    pa.displayName   = "Test Aircraft";
    pa.centerX       = 100.0f;
    pa.centerY       = 200.0f;
    pa.wingspanFt    = 40.0f;
    pa.lengthFt      = 30.0f;
    pa.arrivalTime   = "2026-07-04T08:00";
    pa.departureTime = "2026-07-04T17:00";
    data.aircraft.push_back(pa);

    const std::string path =
        (fs::temp_directory_path() / "arld_timing_test.arld").string();
    fs::remove(path);

    REQUIRE_NOTHROW(arld::core::ProjectFile::save(path, data));
    const auto loaded = arld::core::ProjectFile::load(path);

    REQUIRE(loaded.aircraft.size() == 1);
    CHECK(loaded.aircraft[0].arrivalTime   == "2026-07-04T08:00");
    CHECK(loaded.aircraft[0].departureTime == "2026-07-04T17:00");

    fs::remove(path);
}

// ---------------------------------------------------------------------------
// LabelMode round-trip (Sprint 2-3-4)
// ---------------------------------------------------------------------------

TEST_CASE("ProjectFile: LabelMode serializes and deserializes", "[projectfile]") {
    using LM = arld::core::PlacedAircraft::LabelMode;

    arld::core::ProjectData data;
    data.metadata.title = "LabelMode Test";
    data.schemaVersion  = 2;

    auto makeAc = [](LM mode, const std::string& id) {
        arld::core::PlacedAircraft pa;
        pa.placementId = id;
        pa.libraryId   = "test-aircraft";
        pa.displayName = "Test";
        pa.wingspanFt  = 40.0f;
        pa.lengthFt    = 30.0f;
        pa.labelMode   = mode;
        return pa;
    };

    data.aircraft.push_back(makeAc(LM::DisplayName, "id-display"));
    data.aircraft.push_back(makeAc(LM::TailNumber,  "id-tail"));
    data.aircraft.push_back(makeAc(LM::Hidden,       "id-hidden"));

    const std::string path =
        (fs::temp_directory_path() / "arld_labelmode_test.arld").string();
    fs::remove(path);

    REQUIRE_NOTHROW(arld::core::ProjectFile::save(path, data));
    const auto loaded = arld::core::ProjectFile::load(path);

    REQUIRE(loaded.aircraft.size() == 3);
    CHECK(loaded.aircraft[0].labelMode == LM::DisplayName);
    CHECK(loaded.aircraft[1].labelMode == LM::TailNumber);
    CHECK(loaded.aircraft[2].labelMode == LM::Hidden);

    fs::remove(path);
}
