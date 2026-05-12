#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <arld/core/AircraftLibraryParser.h>
#include <arld/core/ProjectFile.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helper: build a minimal valid aircraft JSON string
// ---------------------------------------------------------------------------
static std::string makeAircraftJson(int idx) {
    std::ostringstream oss;
    oss << R"({
  "id": "test-aircraft-)" << idx << R"(",
  "display_name": "Test Aircraft )" << idx << R"(",
  "manufacturer": "Test Co",
  "model": "T-)" << idx << R"(",
  "category": "GENERAL_AVIATION",
  "wingspan_ft": 35.0,
  "length_ft": 28.0,
  "tail_height_ft": 9.0,
  "prop_arc_ft": 4.0,
  "default_display_type": "static_display",
  "silhouette_svg": "test-aircraft-)" << idx << R"(.svg",
  "data_sources": ["test source"]
})";
    return oss.str();
}

// ---------------------------------------------------------------------------
// Helper: build a 200-aircraft ProjectData and save to temp file
// ---------------------------------------------------------------------------
static std::string build200AircraftFile() {
    arld::core::ProjectData data;
    data.metadata.title = "Bench Layout";
    data.boundary.vertices = {{0.0f, 0.0f}, {5000.0f, 0.0f},
                               {5000.0f, 3000.0f}, {0.0f, 3000.0f}};
    data.boundary.closed = true;

    for (int i = 0; i < 200; ++i) {
        arld::core::PlacedAircraft pa;
        pa.placementId  = arld::core::ProjectFile::generateUuid();
        pa.libraryId    = "test-aircraft-" + std::to_string(i);
        pa.displayName  = "Test Aircraft " + std::to_string(i);
        pa.centerX      = static_cast<float>((i % 20) * 250 + 125);
        pa.centerY      = static_cast<float>((i / 20) * 300 + 150);
        pa.rotationDeg  = 0.0f;
        pa.wingspanFt   = 35.0f;
        pa.lengthFt     = 28.0f;
        pa.displayType  = arld::core::DisplayType::StaticDisplay;
        data.aircraft.push_back(pa);
    }

    const std::string tmpPath =
        (fs::temp_directory_path() / "arld_bench_200ac.arld").string();
    arld::core::ProjectFile::save(tmpPath, data);
    return tmpPath;
}

// ---------------------------------------------------------------------------
// Benchmarks ([.bench] tag — excluded from normal test runs)
// ---------------------------------------------------------------------------

TEST_CASE("bench_library_load", "[.bench]") {
    // Pre-generate 150 entry JSON strings
    std::vector<std::string> entryJsons;
    entryJsons.reserve(150);
    for (int i = 0; i < 150; ++i)
        entryJsons.push_back(makeAircraftJson(i));

    BENCHMARK("AircraftLibraryParser: parse 150 entry JSON strings") {
        int count = 0;
        for (const auto& json : entryJsons) {
            auto entry = arld::core::AircraftLibraryParser::parseEntry(json);
            ++count;
        }
        return count;
    };
}

TEST_CASE("bench_project_open_200ac", "[.bench]") {
    // Build the file once outside the benchmark loop
    const std::string tmpPath = build200AircraftFile();

    BENCHMARK("ProjectFile::load 200-aircraft layout") {
        return arld::core::ProjectFile::load(tmpPath);
    };

    fs::remove(tmpPath);
}
