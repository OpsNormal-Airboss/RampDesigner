// NOTE: No Qt headers — pure C++20 + standard library only.
#include <arld/export/AircraftManifestExporter.h>
#include <arld/core/ProjectFile.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace arld::export_ {

namespace {

// CSV-escape a string: wrap in quotes if it contains comma, quote, or newline.
static std::string csvEscape(const std::string& s) {
    bool needsQuotes = (s.find(',') != std::string::npos ||
                        s.find('"') != std::string::npos ||
                        s.find('\n') != std::string::npos);
    if (!needsQuotes) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else          out += c;
    }
    out += '"';
    return out;
}

} // anonymous namespace

void AircraftManifestExporter::exportCsv(const arld::core::ProjectData& data,
                                          const std::string& outputPath) {
    std::ofstream ofs(outputPath);
    if (!ofs.is_open())
        throw std::runtime_error("AircraftManifestExporter: cannot open: " + outputPath);

    // Header row
    ofs << "Placement ID,Library ID,Display Name,Tail Number,Owner,"
           "Fuel Type,Hazmat,Display Type,Center X (ft),Center Y (ft),Heading (deg)\n";

    for (const auto& ac : data.aircraft) {
        const std::string dtStr = arld::core::displayTypeToString(ac.displayType);
        ofs << csvEscape(ac.placementId) << ","
            << csvEscape(ac.libraryId)   << ","
            << csvEscape(ac.displayName) << ","
            << csvEscape(ac.tailNumber)  << ","
            << csvEscape(ac.owner)       << ","
            << csvEscape(ac.fuelType)    << ","
            << (ac.hasHazmat ? "Yes" : "No") << ","
            << csvEscape(dtStr)          << ","
            << ac.centerX               << ","
            << ac.centerY               << ","
            << ac.rotationDeg           << "\n";
    }

    if (!ofs.good())
        throw std::runtime_error("AircraftManifestExporter: write error: " + outputPath);
}

} // namespace arld::export_
