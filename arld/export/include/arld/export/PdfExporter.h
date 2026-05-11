#pragma once
#include <arld/export/IExporter.h>

namespace arld::export_ {

/// PDF exporter — implementation via libharu (guarded by HAVE_LIBHARU).
/// Falls back to empty stub when libharu is not available.
class PdfExporter : public IExporter {
public:
    void exportLayout(const arld::core::ProjectData& data,
                      const std::string& outputPath,
                      const ExportOptions& options = ExportOptions{}) override;
};

} // namespace arld::export_
