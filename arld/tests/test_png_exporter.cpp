#include <catch2/catch_test_macros.hpp>
#include <arld/export/PngExporter.h>
#include <arld/export/JpegExporter.h>
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
    data.metadata.title     = "Test Show";
    data.metadata.showDate  = "2026-07-04";
    data.metadata.showVenue = "Oshkosh, WI";

    // Simple boundary
    data.boundary.vertices = {{0.0f, 0.0f}, {500.0f, 0.0f},
                               {500.0f, 300.0f}, {0.0f, 300.0f}};
    data.boundary.closed = true;

    // A couple of aircraft
    arld::core::PlacedAircraft a1;
    a1.placementId = "test-001";
    a1.libraryId   = "north-american-p51-d";
    a1.displayName = "P-51D";
    a1.tailNumber  = "N151AF";
    a1.centerX     = 100.0f; a1.centerY = 100.0f;
    a1.wingspanFt  = 37.0f;  a1.lengthFt = 32.2f;
    a1.displayType = arld::core::DisplayType::WarbirdHeritage;
    data.aircraft.push_back(a1);

    arld::core::PlacedAircraft a2;
    a2.placementId = "test-002";
    a2.libraryId   = "boeing-b17-g";
    a2.displayName = "B-17G";
    a2.centerX     = 200.0f; a2.centerY = 150.0f;
    a2.wingspanFt  = 103.9f; a2.lengthFt = 74.4f;
    a2.displayType = arld::core::DisplayType::WarbirdHeritage;
    data.aircraft.push_back(a2);

    return data;
}

// ---------------------------------------------------------------------------
// PngExporter tests
// ---------------------------------------------------------------------------

TEST_CASE("PngExporter creates a file at the output path", "[png]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_output.png").string();
    fs::remove(outPath);

    arld::export_::PngExporter exporter;
    auto data = makeSampleProject();
    REQUIRE_NOTHROW(exporter.exportLayout(data, outPath));
    CHECK(fs::exists(outPath));

    fs::remove(outPath);
}

TEST_CASE("PngExporter creates a non-empty file", "[png]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_nonempty.png").string();
    fs::remove(outPath);

    arld::export_::PngExporter exporter;
    auto data = makeSampleProject();
    exporter.exportLayout(data, outPath);

    const auto size = fs::file_size(outPath);
    CHECK(size > 0);

    fs::remove(outPath);
}

TEST_CASE("PngExporter output starts with PNG magic bytes", "[png]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_magic.png").string();
    fs::remove(outPath);

    arld::export_::PngExporter exporter;
    auto data = makeSampleProject();
    exporter.exportLayout(data, outPath);

    std::ifstream f(outPath, std::ios::binary);
    REQUIRE(f.is_open());
    unsigned char magic[8] = {};
    f.read(reinterpret_cast<char*>(magic), 8);
    // PNG signature: 0x89 0x50 0x4E 0x47 0x0D 0x0A 0x1A 0x0A
    CHECK(magic[0] == 0x89);
    CHECK(magic[1] == 'P');
    CHECK(magic[2] == 'N');
    CHECK(magic[3] == 'G');

    fs::remove(outPath);
}

TEST_CASE("PngExporter higher DPI produces larger file", "[png]") {
    const std::string outLow  = (fs::temp_directory_path() / "arld_test_low_dpi.png").string();
    const std::string outHigh = (fs::temp_directory_path() / "arld_test_high_dpi.png").string();
    fs::remove(outLow);
    fs::remove(outHigh);

    arld::export_::PngExporter exporter;
    auto data = makeSampleProject();

    arld::export_::ExportOptions optsLow;
    optsLow.dpi = 96;
    exporter.exportLayout(data, outLow, optsLow);

    arld::export_::ExportOptions optsHigh;
    optsHigh.dpi = 300;
    exporter.exportLayout(data, outHigh, optsHigh);

    const auto sizeLow  = fs::file_size(outLow);
    const auto sizeHigh = fs::file_size(outHigh);
    CHECK(sizeHigh > sizeLow);

    fs::remove(outLow);
    fs::remove(outHigh);
}

// ---------------------------------------------------------------------------
// JpegExporter tests
// ---------------------------------------------------------------------------

TEST_CASE("JpegExporter creates a file at the output path", "[jpeg]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_output.jpg").string();
    fs::remove(outPath);

    arld::export_::JpegExporter exporter;
    auto data = makeSampleProject();
    REQUIRE_NOTHROW(exporter.exportLayout(data, outPath));
    CHECK(fs::exists(outPath));

    fs::remove(outPath);
}

TEST_CASE("JpegExporter creates a non-empty file", "[jpeg]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_nonempty.jpg").string();
    fs::remove(outPath);

    arld::export_::JpegExporter exporter;
    auto data = makeSampleProject();
    exporter.exportLayout(data, outPath);

    const auto size = fs::file_size(outPath);
    CHECK(size > 0);

    fs::remove(outPath);
}

TEST_CASE("JpegExporter output starts with JPEG magic bytes", "[jpeg]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_magic.jpg").string();
    fs::remove(outPath);

    arld::export_::JpegExporter exporter;
    auto data = makeSampleProject();
    exporter.exportLayout(data, outPath);

    std::ifstream f(outPath, std::ios::binary);
    REQUIRE(f.is_open());
    unsigned char magic[2] = {};
    f.read(reinterpret_cast<char*>(magic), 2);
    CHECK(magic[0] == 0xFF);
    CHECK(magic[1] == 0xD8);

    fs::remove(outPath);
}

TEST_CASE("JpegExporter quality 60 produces smaller file than quality 100", "[jpeg]") {
    const std::string outLow  = (fs::temp_directory_path() / "arld_test_jpeg_q60.jpg").string();
    const std::string outHigh = (fs::temp_directory_path() / "arld_test_jpeg_q100.jpg").string();
    fs::remove(outLow);
    fs::remove(outHigh);

    arld::export_::JpegExporter exporter;
    auto data = makeSampleProject();

    arld::export_::ExportOptions optsLow;
    optsLow.jpegQuality = 60;
    exporter.exportLayout(data, outLow, optsLow);

    arld::export_::ExportOptions optsHigh;
    optsHigh.jpegQuality = 100;
    exporter.exportLayout(data, outHigh, optsHigh);

    const auto sizeLow  = fs::file_size(outLow);
    const auto sizeHigh = fs::file_size(outHigh);
    CHECK(sizeLow < sizeHigh);

    fs::remove(outLow);
    fs::remove(outHigh);
}

TEST_CASE("JpegExporter dimension cap fires for huge layout", "[jpeg]") {
    // 36000ft wide x 200ft tall at 96 DPI => 36000px wide — above the 32767 cap.
    // Height stays small so the buffer is manageable in a test environment.
    arld::core::ProjectData data;
    data.metadata.title = "Wide Layout";
    data.boundary.vertices = {{0.0f, 0.0f}, {36000.0f, 0.0f},
                               {36000.0f, 200.0f}, {0.0f, 200.0f}};
    data.boundary.closed = true;

    const std::string outPath = (fs::temp_directory_path() / "arld_test_huge.jpg").string();
    fs::remove(outPath);

    arld::export_::JpegExporter exporter;
    arld::export_::ExportOptions opts;
    opts.dpi = 96;  // pixPerFt=1.0 => 36000+200px wide => triggers cap

    // Should not throw — dimension is capped and file is written
    REQUIRE_NOTHROW(exporter.exportLayout(data, outPath, opts));
    CHECK(fs::exists(outPath));
    CHECK(fs::file_size(outPath) > 0);

    // Verify JPEG magic bytes — file must not be corrupt
    std::ifstream f(outPath, std::ios::binary);
    REQUIRE(f.is_open());
    unsigned char magic[2] = {};
    f.read(reinterpret_cast<char*>(magic), 2);
    CHECK(magic[0] == 0xFF);
    CHECK(magic[1] == 0xD8);

    fs::remove(outPath);
}
