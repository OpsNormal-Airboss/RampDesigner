#include <arld/export/PdfExporter.h>
#include <fstream>

namespace arld::export_ {

void PdfExporter::exportLayout(const arld::core::ProjectData& /*data*/,
                               const std::string& outputPath) {
    // empty stub — Phase 1+ implementation via libharu
    std::ofstream f(outputPath);
}

} // namespace arld::export_
