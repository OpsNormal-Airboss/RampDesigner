#pragma once
#include <arld/export/IExporter.h>

namespace arld::export_ {

/// Exports an ARLD layout to a stand-alone SVG file.
/// Pure C++ implementation — no Qt dependency.
class SvgExporter : public IExporter {
public:
    void exportLayout(const arld::core::ProjectData& data,
                      const std::string& outputPath,
                      const ExportOptions& options = ExportOptions{}) override;
};

} // namespace arld::export_
