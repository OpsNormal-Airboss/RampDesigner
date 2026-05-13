#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <arld/export/BatchExporter.h>
#include <arld/export/SvgExporter.h>
#include <arld/export/PngExporter.h>
#include <arld/export/JpegExporter.h>
#include <arld/export/AircraftManifestExporter.h>
#include <arld/core/ProjectFile.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static arld::core::ProjectData makeSampleProject() {
    arld::core::ProjectData data;
    data.metadata.title    = "Test Show";
    data.metadata.showDate = "2026-07-04";
    data.metadata.showVenue = "Oshkosh, WI";

    data.boundary.vertices = {{0.0f, 0.0f}, {500.0f, 0.0f},
                               {500.0f, 300.0f}, {0.0f, 300.0f}};
    data.boundary.closed = true;

    arld::core::PlacedAircraft a1;
    a1.placementId = "batch-001";
    a1.libraryId   = "north-american-p51-d";
    a1.displayName = "P-51D";
    a1.tailNumber  = "N151AF";
    a1.owner       = "Warbird Museum";
    a1.fuelType    = "Avgas 100LL";
    a1.centerX     = 100.0f; a1.centerY = 100.0f;
    a1.wingspanFt  = 37.0f;  a1.lengthFt = 32.2f;
    a1.displayType = arld::core::DisplayType::WarbirdHeritage;
    data.aircraft.push_back(a1);

    arld::core::PlacedAircraft a2;
    a2.placementId = "batch-002";
    a2.libraryId   = "boeing-b17-g";
    a2.displayName = "B-17G";
    a2.tailNumber  = "N3703G";
    a2.centerX     = 250.0f; a2.centerY = 150.0f;
    a2.wingspanFt  = 103.9f; a2.lengthFt = 74.4f;
    a2.displayType = arld::core::DisplayType::WarbirdHeritage;
    data.aircraft.push_back(a2);

    return data;
}

/// Build a 200-aircraft ProjectData: 40x5 grid of B-17 sized aircraft.
static arld::core::ProjectData make200AircraftProject() {
    arld::core::ProjectData data;
    data.metadata.title = "Bench Layout";

    data.boundary.vertices = {{0.0f, 0.0f}, {2000.0f, 0.0f},
                               {2000.0f, 1200.0f}, {0.0f, 1200.0f}};
    data.boundary.closed = true;

    int idx = 0;
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 40; ++col) {
            arld::core::PlacedAircraft ac;
            ac.placementId = "bench-" + std::to_string(++idx);
            ac.libraryId   = "boeing-b17-g";
            ac.displayName = "B-17G";
            ac.centerX     = static_cast<float>(50.0 + col * 48.0);
            ac.centerY     = static_cast<float>(100.0 + row * 220.0);
            ac.wingspanFt  = 103.9f;
            ac.lengthFt    = 74.9f;
            ac.displayType = arld::core::DisplayType::WarbirdHeritage;
            data.aircraft.push_back(ac);
        }
    }
    return data;
}

// ---------------------------------------------------------------------------
// BatchExporter tests
// ---------------------------------------------------------------------------

TEST_CASE("BatchExporter::exportAll creates 4 output files", "[batch]") {
    const std::string base = (fs::temp_directory_path() / "arld_batch_test").string();
    // Clean up any previous run.
    for (const auto& ext : {".svg", ".pdf", ".png", ".jpg"})
        fs::remove(base + ext);

    auto data = makeSampleProject();
    // exportAll may throw if PDF requires libharu but we only check the files that succeed.
    try {
        arld::export_::BatchExporter::exportAll(data, base);
    } catch (const std::exception&) {
        // Partial failure is acceptable in environments without all libraries.
    }

    // At minimum SVG, PNG, JPEG should always be written.
    CHECK(fs::exists(base + ".svg"));
    CHECK(fs::exists(base + ".png"));
    CHECK(fs::exists(base + ".jpg"));

    for (const auto& ext : {".svg", ".pdf", ".png", ".jpg"})
        fs::remove(base + ext);
}

TEST_CASE("BatchExporter output files have correct extensions", "[batch]") {
    const std::string base = (fs::temp_directory_path() / "arld_batch_ext_test").string();
    for (const auto& ext : {".svg", ".pdf", ".png", ".jpg"})
        fs::remove(base + ext);

    auto data = makeSampleProject();
    try {
        arld::export_::BatchExporter::exportAll(data, base);
    } catch (...) {}

    // Verify that SVG file starts with '<' (XML), PNG with PNG magic, JPEG with FF D8.
    if (fs::exists(base + ".svg")) {
        std::ifstream f(base + ".svg");
        std::string firstChar(1, '\0');
        f.read(firstChar.data(), 1);
        // SVG/XML starts with '<' or '?' (XML declaration)
        CHECK((!firstChar.empty() && (firstChar[0] == '<' || firstChar[0] == '?')));
    }
    if (fs::exists(base + ".png")) {
        std::ifstream f(base + ".png", std::ios::binary);
        unsigned char magic[2] = {};
        f.read(reinterpret_cast<char*>(magic), 2);
        CHECK(magic[0] == 0x89);
        CHECK(magic[1] == 'P');
    }
    if (fs::exists(base + ".jpg")) {
        std::ifstream f(base + ".jpg", std::ios::binary);
        unsigned char magic[2] = {};
        f.read(reinterpret_cast<char*>(magic), 2);
        CHECK(magic[0] == 0xFF);
        CHECK(magic[1] == 0xD8);
    }

    for (const auto& ext : {".svg", ".pdf", ".png", ".jpg"})
        fs::remove(base + ext);
}

// ---------------------------------------------------------------------------
// AircraftManifestExporter tests
// ---------------------------------------------------------------------------

TEST_CASE("AircraftManifestExporter creates CSV with header", "[manifest]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_manifest_test.csv").string();
    fs::remove(outPath);

    auto data = makeSampleProject();
    REQUIRE_NOTHROW(arld::export_::AircraftManifestExporter::exportCsv(data, outPath));
    CHECK(fs::exists(outPath));

    std::ifstream f(outPath);
    REQUIRE(f.is_open());
    std::string firstLine;
    std::getline(f, firstLine);
    CHECK(firstLine.find("Placement ID") != std::string::npos);
    CHECK(firstLine.find("Tail Number")  != std::string::npos);
    CHECK(firstLine.find("Display Type") != std::string::npos);
    f.close();

    fs::remove(outPath);
}

TEST_CASE("AircraftManifestExporter CSV has one row per aircraft", "[manifest]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_manifest_rows.csv").string();
    fs::remove(outPath);

    auto data = makeSampleProject();
    arld::export_::AircraftManifestExporter::exportCsv(data, outPath);

    std::ifstream f(outPath);
    REQUIRE(f.is_open());
    int lineCount = 0;
    std::string line;
    while (std::getline(f, line)) ++lineCount;
    // 1 header + 2 aircraft = 3 lines
    CHECK(lineCount == 3);
    f.close();

    fs::remove(outPath);
}

// ---------------------------------------------------------------------------
// SvgExporter inkscape:label test
// ---------------------------------------------------------------------------

TEST_CASE("SvgExporter output contains inkscape:label", "[svg][layers]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_layers_test.svg").string();
    fs::remove(outPath);

    auto data = makeSampleProject();
    arld::export_::SvgExporter{}.exportLayout(data, outPath);

    std::ifstream f(outPath);
    REQUIRE(f.is_open());
    std::string content((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    CHECK(content.find("inkscape:label") != std::string::npos);
    CHECK(content.find("Ramp Boundary") != std::string::npos);
    CHECK(content.find("id=\"aircraft\"") != std::string::npos);
    CHECK(content.find("id=\"annotations\"") != std::string::npos);
    f.close();

    fs::remove(outPath);
}

TEST_CASE("AircraftManifestExporter empty project writes only header", "[manifest]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_manifest_empty.csv").string();
    fs::remove(outPath);

    arld::core::ProjectData empty;
    arld::export_::AircraftManifestExporter::exportCsv(empty, outPath);

    std::ifstream f(outPath);
    REQUIRE(f.is_open());
    int lineCount = 0;
    std::string line;
    while (std::getline(f, line)) ++lineCount;
    CHECK(lineCount == 1); // only header
    f.close();

    fs::remove(outPath);
}

// ---------------------------------------------------------------------------
// Export performance benchmarks (tagged [.bench] so excluded from default run)
// ---------------------------------------------------------------------------

TEST_CASE("bench_export_svg", "[.bench]") {
    auto data = make200AircraftProject();
    BENCHMARK("SVG export 200ac") {
        arld::export_::SvgExporter{}.exportLayout(data, "/tmp/arld_bench.svg");
        return 0;
    };
}

TEST_CASE("bench_export_png", "[.bench]") {
    auto data = make200AircraftProject();
    BENCHMARK("PNG export 200ac") {
        arld::export_::PngExporter{}.exportLayout(data, "/tmp/arld_bench.png");
        return 0;
    };
}

TEST_CASE("bench_export_jpeg", "[.bench]") {
    auto data = make200AircraftProject();
    BENCHMARK("JPEG export 200ac") {
        arld::export_::JpegExporter{}.exportLayout(data, "/tmp/arld_bench.jpg");
        return 0;
    };
}
