#include <arld/export/PngExporter.h>
#include <fstream>

namespace arld::export_ {

void PngExporter::exportLayout(const arld::core::ProjectData& /*data*/,
                               const std::string& outputPath,
                               const ExportOptions& /*options*/) {
    // empty stub — Phase 1+ implementation via stb_image_write
    std::ofstream f(outputPath);
}

} // namespace arld::export_
