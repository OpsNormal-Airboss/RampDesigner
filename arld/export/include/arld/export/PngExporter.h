#pragma once
#include <arld/export/IExporter.h>

namespace arld::export_ {

/// PNG exporter stub — Phase 1+ implementation via stb_image_write.
class PngExporter : public IExporter {
public:
    void exportLayout(const arld::core::ProjectData& data,
                      const std::string& outputPath) override;
};

} // namespace arld::export_
