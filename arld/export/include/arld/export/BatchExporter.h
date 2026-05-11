#pragma once
#include <arld/export/IExporter.h>
#include <string>

namespace arld::export_ {

/// Exports a layout to all 4 formats (SVG, PDF, PNG, JPEG) in one call.
/// Output files: basePath + ".svg", ".pdf", ".png", ".jpg"
class BatchExporter {
public:
    /// Throws std::runtime_error if ALL exports fail; continues with remaining
    /// formats on partial failure and aggregates errors in the exception message.
    static void exportAll(const arld::core::ProjectData& data,
                          const std::string& basePath,
                          const ExportOptions& options = ExportOptions{});
};

} // namespace arld::export_
