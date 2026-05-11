#pragma once
#include <arld/core/ClearanceEngine.h>
#include <arld/core/ProjectFile.h>
#include <string>
#include <vector>

namespace arld::export_ {

/// Exports a violations and overrides report as a UTF-8 CSV file.
/// Pure C++ — no Qt dependency.
class ViolationReportExporter {
public:
    /// Writes a UTF-8 CSV to @p outputPath.
    /// Columns: "Aircraft A","Aircraft B","Severity","Measured Gap (ft)",
    ///          "Required Gap (ft)","Override Justification"
    /// Throws std::runtime_error on I/O failure.
    static void exportCsv(
        const std::vector<arld::core::ViolationResult>& violations,
        const std::vector<arld::core::ClearanceOverride>& overrides,
        const std::string& outputPath);
};

} // namespace arld::export_
