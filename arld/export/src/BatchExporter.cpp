// NOTE: No Qt headers — pure C++20 + standard library only.
#include <arld/export/BatchExporter.h>
#include <arld/export/SvgExporter.h>
#include <arld/export/PdfExporter.h>
#include <arld/export/PngExporter.h>
#include <arld/export/JpegExporter.h>
#include <stdexcept>
#include <string>

namespace arld::export_ {

void BatchExporter::exportAll(const arld::core::ProjectData& data,
                              const std::string& basePath,
                              const ExportOptions& options) {
    std::string lastError;

    // SVG
    try { SvgExporter{}.exportLayout(data, basePath + ".svg", options); }
    catch (const std::exception& e) { lastError += std::string("SVG: ") + e.what() + "\n"; }

    // PDF
    try { PdfExporter{}.exportLayout(data, basePath + ".pdf", options); }
    catch (const std::exception& e) { lastError += std::string("PDF: ") + e.what() + "\n"; }

    // PNG
    try { PngExporter{}.exportLayout(data, basePath + ".png", options); }
    catch (const std::exception& e) { lastError += std::string("PNG: ") + e.what() + "\n"; }

    // JPEG
    try { JpegExporter{}.exportLayout(data, basePath + ".jpg", options); }
    catch (const std::exception& e) { lastError += std::string("JPEG: ") + e.what() + "\n"; }

    if (!lastError.empty())
        throw std::runtime_error("BatchExporter::exportAll partial failure:\n" + lastError);
}

} // namespace arld::export_
