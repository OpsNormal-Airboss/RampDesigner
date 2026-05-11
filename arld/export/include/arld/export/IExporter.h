#pragma once
#include <arld/core/ClearanceEngine.h>
#include <arld/core/ProjectFile.h>
#include <string>
#include <vector>

namespace arld::export_ {

struct ExportOptions {
    // PDF paper size (ignored by SVG/PNG/JPEG exporters)
    enum class PaperSize { Letter, Tabloid, ANSI_C, ANSI_D, ANSI_E, ANSI_E1 };
    enum class Orientation { Portrait, Landscape };

    PaperSize   paperSize   = PaperSize::ANSI_D;
    Orientation orientation = Orientation::Landscape;

    // Optional path to a TrueType font file (Inter, etc.) for PDF embedding.
    // Falls back to Helvetica if empty or file not found.
    std::string fontPath;

    // Optional path to the saved .arld file — used to generate SHA-256 QR code.
    // If empty the QR code block is omitted.
    std::string arldFilePath;

    // PNG/JPEG raster settings (ignored by other exporters)
    int  dpi         = 150;
    int  jpegQuality = 90;

    // Violations report (used by both PdfExporter and ViolationReportExporter)
    bool includeViolations = false;
    std::vector<arld::core::ViolationResult>   violations;
    std::vector<arld::core::ClearanceOverride> overrides;

    // Show metadata for PDF title block (can override data.metadata fields)
    std::string showName;    // defaults to data.metadata.title if empty
    std::string showDate;    // e.g. "2026-07-04"
    std::string showVenue;   // e.g. "Oshkosh, WI"
};

class IExporter {
public:
    virtual ~IExporter() = default;

    /// Export @p data to @p outputPath in the format implemented by this class.
    /// Throws std::runtime_error on failure.
    virtual void exportLayout(const arld::core::ProjectData& data,
                              const std::string& outputPath,
                              const ExportOptions& options = ExportOptions{}) = 0;
};

} // namespace arld::export_
