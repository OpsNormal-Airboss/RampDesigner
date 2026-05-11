#pragma once
#include <arld/export/IExporter.h>

namespace arld::export_ {

/// JPEG exporter stub — Phase 1+ implementation via stb_image_write.
class JpegExporter : public IExporter {
public:
    void exportLayout(const arld::core::ProjectData& data,
                      const std::string& outputPath,
                      const ExportOptions& options = ExportOptions{}) override;
};

} // namespace arld::export_
