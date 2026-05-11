#pragma once
#include <arld/export/IExporter.h>
#include <memory>
#include <vector>

namespace arld::export_ {

/// Composite exporter that runs multiple exporters against the same layout.
class BatchExporter : public IExporter {
public:
    /// Add an exporter to the batch. Exporters run in insertion order.
    void addExporter(std::unique_ptr<IExporter> exp);

    /// Calls exportLayout() on each registered exporter with the same outputPath.
    void exportLayout(const arld::core::ProjectData& data,
                      const std::string& outputPath,
                      const ExportOptions& options = ExportOptions{}) override;

private:
    std::vector<std::unique_ptr<IExporter>> m_exporters;
};

} // namespace arld::export_
