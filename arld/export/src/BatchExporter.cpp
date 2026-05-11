#include <arld/export/BatchExporter.h>

namespace arld::export_ {

void BatchExporter::addExporter(std::unique_ptr<IExporter> exp) {
    m_exporters.push_back(std::move(exp));
}

void BatchExporter::exportLayout(const arld::core::ProjectData& data,
                                 const std::string& outputPath,
                                 const ExportOptions& options) {
    for (auto& exporter : m_exporters) {
        exporter->exportLayout(data, outputPath, options);
    }
}

} // namespace arld::export_
