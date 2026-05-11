#pragma once
#include <arld/core/ProjectFile.h>
#include <string>

namespace arld::export_ {

/// Strategy-pattern interface for all layout exporters.
/// Implementations must not depend on Qt.
class IExporter {
public:
    virtual ~IExporter() = default;

    /// Export @p data to @p outputPath in the format implemented by this class.
    /// Throws std::runtime_error on failure.
    virtual void exportLayout(const arld::core::ProjectData& data,
                              const std::string& outputPath) = 0;
};

} // namespace arld::export_
