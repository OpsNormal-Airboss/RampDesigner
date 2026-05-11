// NOTE: No Qt headers — pure C++20 + standard library only.
#include <arld/export/ViolationReportExporter.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace arld::export_ {

namespace {

std::string severityToString(arld::core::ClearanceSeverity sev) {
    switch (sev) {
        case arld::core::ClearanceSeverity::Advisory:   return "Advisory";
        case arld::core::ClearanceSeverity::Violation:  return "Violation";
        case arld::core::ClearanceSeverity::Overridden: return "Overridden";
        case arld::core::ClearanceSeverity::Clear:      return "Clear";
    }
    return "Unknown";
}

// CSV-escape a field: wrap in quotes if it contains comma, quote, or newline.
std::string csvField(const std::string& s) {
    bool needsQuote = (s.find(',') != std::string::npos
                    || s.find('"') != std::string::npos
                    || s.find('\n') != std::string::npos);
    if (!needsQuote) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else          out += c;
    }
    out += '"';
    return out;
}

std::string fmtFloat(float v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(v));
    return buf;
}

} // anonymous namespace

void ViolationReportExporter::exportCsv(
    const std::vector<arld::core::ViolationResult>& violations,
    const std::vector<arld::core::ClearanceOverride>& overrides,
    const std::string& outputPath)
{
    std::ofstream ofs(outputPath);
    if (!ofs.is_open())
        throw std::runtime_error("ViolationReportExporter: cannot open file: " + outputPath);

    // Header row
    ofs << "Aircraft A,Aircraft B,Severity,"
           "Measured Gap (ft),Required Gap (ft),Override Justification\n";

    // Build a lookup: (idA, idB) -> justification (both orderings)
    auto makeKey = [](const std::string& a, const std::string& b) -> std::string {
        return (a < b) ? (a + "|" + b) : (b + "|" + a);
    };
    std::unordered_map<std::string, std::string> justMap;
    for (const auto& ov : overrides) {
        justMap[makeKey(ov.placementIdA, ov.placementIdB)] = ov.justification;
    }

    for (const auto& v : violations) {
        std::string just;
        auto it = justMap.find(makeKey(v.idA, v.idB));
        if (it != justMap.end()) just = it->second;

        ofs << csvField(v.idA) << ","
            << csvField(v.idB) << ","
            << csvField(severityToString(v.severity)) << ","
            << csvField(fmtFloat(v.separationFt)) << ","
            << csvField(fmtFloat(v.requiredFt)) << ","
            << csvField(just) << "\n";
    }

    if (!ofs.good())
        throw std::runtime_error("ViolationReportExporter: write error: " + outputPath);
}

} // namespace arld::export_
