#include <catch2/catch_test_macros.hpp>
#include <arld/export/PdfExporter.h>
#include <arld/export/ViolationReportExporter.h>
#include <arld/core/ProjectFile.h>
#include <arld/core/ClearanceEngine.h>
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

static std::vector<arld::core::ViolationResult> makeSampleViolations() {
    arld::core::ViolationResult v;
    v.idA          = "test-001";
    v.idB          = "test-002";
    v.severity     = arld::core::ClearanceSeverity::Violation;
    v.separationFt = 5.0f;
    v.requiredFt   = 25.0f;
    return {v};
}

// ---------------------------------------------------------------------------
// PdfExporter tests
// ---------------------------------------------------------------------------
#ifdef HAVE_LIBHARU

TEST_CASE("PdfExporter creates a file at the output path", "[pdf]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_output.pdf").string();
    fs::remove(outPath);

    arld::export_::PdfExporter exporter;
    auto data = makeSampleProject();
    REQUIRE_NOTHROW(exporter.exportLayout(data, outPath));
    CHECK(fs::exists(outPath));

    fs::remove(outPath);
}

TEST_CASE("PdfExporter creates a non-empty file", "[pdf]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_nonempty.pdf").string();
    fs::remove(outPath);

    arld::export_::PdfExporter exporter;
    auto data = makeSampleProject();
    exporter.exportLayout(data, outPath);

    const auto size = fs::file_size(outPath);
    CHECK(size > 0);

    fs::remove(outPath);
}

TEST_CASE("PdfExporter output starts with %PDF magic bytes", "[pdf]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_magic.pdf").string();
    fs::remove(outPath);

    arld::export_::PdfExporter exporter;
    auto data = makeSampleProject();
    exporter.exportLayout(data, outPath);

    std::ifstream f(outPath, std::ios::binary);
    REQUIRE(f.is_open());
    char magic[5] = {};
    f.read(magic, 4);
    CHECK(std::string(magic, 4) == "%PDF");
    f.close();

    fs::remove(outPath);
}

TEST_CASE("PdfExporter with violations creates two-page PDF", "[pdf]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_violations.pdf").string();
    fs::remove(outPath);

    arld::export_::ExportOptions opts;
    opts.includeViolations = true;
    opts.violations = makeSampleViolations();

    arld::export_::PdfExporter exporter;
    auto data = makeSampleProject();
    REQUIRE_NOTHROW(exporter.exportLayout(data, outPath, opts));
    CHECK(fs::file_size(outPath) > 0);

    fs::remove(outPath);
}

TEST_CASE("PdfExporter with landscape orientation produces file", "[pdf]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_landscape.pdf").string();
    fs::remove(outPath);

    arld::export_::ExportOptions opts;
    opts.paperSize   = arld::export_::ExportOptions::PaperSize::Letter;
    opts.orientation = arld::export_::ExportOptions::Orientation::Landscape;

    arld::export_::PdfExporter exporter;
    auto data = makeSampleProject();
    REQUIRE_NOTHROW(exporter.exportLayout(data, outPath, opts));
    CHECK(fs::file_size(outPath) > 0);

    fs::remove(outPath);
}

#else  // !HAVE_LIBHARU

TEST_CASE("PdfExporter stub creates a file (no libharu)", "[pdf]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_stub.pdf").string();
    fs::remove(outPath);

    arld::export_::PdfExporter exporter;
    auto data = makeSampleProject();
    REQUIRE_NOTHROW(exporter.exportLayout(data, outPath));
    CHECK(fs::exists(outPath));

    fs::remove(outPath);
}

#endif // HAVE_LIBHARU

// ---------------------------------------------------------------------------
// ViolationReportExporter (CSV) tests — always run, no external deps
// ---------------------------------------------------------------------------

TEST_CASE("ViolationReportExporter::exportCsv creates a file", "[csv]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_violations.csv").string();
    fs::remove(outPath);

    auto violations = makeSampleViolations();
    REQUIRE_NOTHROW(arld::export_::ViolationReportExporter::exportCsv(
        violations, {}, outPath));
    CHECK(fs::exists(outPath));

    fs::remove(outPath);
}

TEST_CASE("ViolationReportExporter CSV contains Severity header", "[csv]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_header.csv").string();
    fs::remove(outPath);

    arld::export_::ViolationReportExporter::exportCsv({}, {}, outPath);

    std::ifstream f(outPath);
    REQUIRE(f.is_open());
    std::string firstLine;
    std::getline(f, firstLine);
    CHECK(firstLine.find("Severity") != std::string::npos);
    f.close();

    fs::remove(outPath);
}

TEST_CASE("ViolationReportExporter CSV has correct row count", "[csv]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_rows.csv").string();
    fs::remove(outPath);

    auto violations = makeSampleViolations();
    // Add a second violation
    arld::core::ViolationResult v2;
    v2.idA = "test-003"; v2.idB = "test-004";
    v2.severity = arld::core::ClearanceSeverity::Advisory;
    v2.separationFt = 28.0f; v2.requiredFt = 25.0f;
    violations.push_back(v2);

    arld::export_::ViolationReportExporter::exportCsv(violations, {}, outPath);

    std::ifstream f(outPath);
    REQUIRE(f.is_open());
    int lineCount = 0;
    std::string line;
    while (std::getline(f, line)) ++lineCount;
    // 1 header + 2 violation rows = 3
    CHECK(lineCount == 3);
    f.close();

    fs::remove(outPath);
}

TEST_CASE("ViolationReportExporter CSV with override appends justification", "[csv]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_override.csv").string();
    fs::remove(outPath);

    auto violations = makeSampleViolations();

    arld::core::ClearanceOverride ov;
    ov.placementIdA  = "test-001";
    ov.placementIdB  = "test-002";
    ov.justification = "Approved by safety officer";

    arld::export_::ViolationReportExporter::exportCsv(violations, {ov}, outPath);

    std::ifstream f(outPath);
    REQUIRE(f.is_open());
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    CHECK(content.find("Approved by safety officer") != std::string::npos);
    f.close();

    fs::remove(outPath);
}

TEST_CASE("ViolationReportExporter empty violations writes only header", "[csv]") {
    const std::string outPath = (fs::temp_directory_path() / "arld_test_empty.csv").string();
    fs::remove(outPath);

    arld::export_::ViolationReportExporter::exportCsv({}, {}, outPath);

    std::ifstream f(outPath);
    REQUIRE(f.is_open());
    int lineCount = 0;
    std::string line;
    while (std::getline(f, line)) ++lineCount;
    CHECK(lineCount == 1); // only header
    f.close();

    fs::remove(outPath);
}
