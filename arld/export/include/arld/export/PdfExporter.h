#pragma once
#include <arld/export/IExporter.h>

namespace arld::export_ {

/// PDF exporter stub — Phase 1+ implementation via libharu.
class PdfExporter : public IExporter {
public:
    void exportLayout(const arld::core::ProjectData& data,
                      const std::string& outputPath) override;
};

} // namespace arld::export_
