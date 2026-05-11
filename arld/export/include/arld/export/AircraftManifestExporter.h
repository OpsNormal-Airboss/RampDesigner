#pragma once
#include <arld/core/ProjectFile.h>
#include <string>

namespace arld::export_ {

/// Exports a per-aircraft metadata manifest as CSV.
/// Header: Placement ID, Library ID, Display Name, Tail Number, Owner,
///         Fuel Type, Hazmat, Display Type, Center X (ft), Center Y (ft),
///         Heading (deg)
class AircraftManifestExporter {
public:
    static void exportCsv(const arld::core::ProjectData& data,
                          const std::string& outputPath);
};

} // namespace arld::export_
