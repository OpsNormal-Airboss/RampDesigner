// NOTE: No Qt headers — pure C++20 + standard library only.
#include <arld/export/SvgExporter.h>
#include <arld/core/ProjectFile.h>   // PlacedAircraft, ProjectData, displayTypeToString
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace arld::export_ {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

// Format a float with up to 3 decimal places, stripping trailing zeros.
std::string fmt(double v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.3f", v);
    // Strip trailing zeros after decimal point.
    std::string s = buf;
    if (s.find('.') != std::string::npos) {
        size_t last = s.find_last_not_of('0');
        if (last != std::string::npos && s[last] == '.')
            s.erase(last + 1); // leave the dot-less integer
        else if (last != std::string::npos)
            s.erase(last + 1);
    }
    return s;
}

// XML-escape a string for use in attribute values and text content.
std::string xmlEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += c;
        }
    }
    return out;
}

// Fill color per display type.
const char* displayTypeColor(const std::string& dt) {
    if (dt == "static_display")        return "#4477AA";
    if (dt == "warbird_heritage")      return "#447744";
    if (dt == "taxi_only")             return "#AA7744";
    if (dt == "military_static")       return "#445566";
    if (dt == "hot_ramp")              return "#AA4433";
    if (dt == "media_photo_platform")  return "#774477";
    if (dt == "ramp_show")             return "#447788";
    return "#888888"; // fallback
}

struct BBox {
    double minX, minY, maxX, maxY;
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// SvgExporter::exportLayout
// ---------------------------------------------------------------------------
void SvgExporter::exportLayout(const arld::core::ProjectData& data,
                                const std::string& outputPath,
                                const ExportOptions& /*options*/) {
    constexpr double kPadding = 100.0;   // feet of margin around content
    constexpr double kMaxPx   = 2000.0;  // max SVG dimension (px)

    // ------------------------------------------------------------------
    // 1. Compute bounding box from boundary vertices + aircraft footprints.
    // ------------------------------------------------------------------
    BBox bbox { DBL_MAX, DBL_MAX, -DBL_MAX, -DBL_MAX };

    auto expandBBox = [&](double x, double y) {
        bbox.minX = std::min(bbox.minX, x);
        bbox.minY = std::min(bbox.minY, y);
        bbox.maxX = std::max(bbox.maxX, x);
        bbox.maxY = std::max(bbox.maxY, y);
    };

    // Boundary vertices
    for (const auto& [x, y] : data.boundary.vertices) {
        expandBBox(x, y);
    }

    // Aircraft — expand by worst-case rotated extent (half-diagonal of footprint)
    for (const auto& ac : data.aircraft) {
        // Conservative bounding circle: half-diagonal of the footprint rect
        const double hw = ac.wingspanFt / 2.0;
        const double hl = ac.lengthFt   / 2.0;
        const double radius = std::sqrt(hw * hw + hl * hl);
        expandBBox(ac.centerX - radius, ac.centerY - radius);
        expandBBox(ac.centerX + radius, ac.centerY + radius);
    }

    // If nothing was in the scene at all, use a small default viewport.
    if (bbox.minX == DBL_MAX) {
        bbox = { -100.0, -100.0, 100.0, 100.0 };
    }

    // Add padding.
    bbox.minX -= kPadding;
    bbox.minY -= kPadding;
    bbox.maxX += kPadding;
    bbox.maxY += kPadding;

    const double worldW = bbox.maxX - bbox.minX;
    const double worldH = bbox.maxY - bbox.minY;

    // ------------------------------------------------------------------
    // 2. Determine SVG dimensions (1 ft = 1 px, capped at kMaxPx).
    // ------------------------------------------------------------------
    double scale = 1.0;  // px per ft
    if (worldW > kMaxPx || worldH > kMaxPx) {
        scale = kMaxPx / std::max(worldW, worldH);
    }

    const double svgW = worldW * scale;
    const double svgH = worldH * scale;

    // Helper: world-ft → SVG-px
    auto wx = [&](double x) { return (x - bbox.minX) * scale; };
    auto wy = [&](double y) { return (y - bbox.minY) * scale; };

    // ------------------------------------------------------------------
    // 3. Build SVG string.
    // ------------------------------------------------------------------
    std::ostringstream svg;

    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<svg xmlns=\"http://www.w3.org/2000/svg\""
        << " xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\""
        << " width=\""  << fmt(svgW) << "\""
        << " height=\"" << fmt(svgH) << "\""
        << " viewBox=\"0 0 " << fmt(svgW) << " " << fmt(svgH) << "\""
        << ">\n";

    // Background
    svg << "  <rect width=\"100%\" height=\"100%\" fill=\"#F8F8F0\"/>\n";

    // -- Ramp boundary layer --
    svg << "  <g id=\"boundary\" inkscape:label=\"Ramp Boundary\">\n";
    if (!data.boundary.vertices.empty()) {
        svg << "    <polygon points=\"";
        bool first = true;
        for (const auto& [x, y] : data.boundary.vertices) {
            if (!first) svg << " ";
            svg << fmt(wx(x)) << "," << fmt(wy(y));
            first = false;
        }
        svg << "\""
            << " fill=\"#E8F5E8\""
            << " stroke=\"#338833\""
            << " stroke-width=\"" << fmt(3.0 * scale) << "\""
            << "/>\n";
    }
    svg << "  </g><!-- boundary -->\n";

    // -- Aircraft layer --
    svg << "  <g id=\"aircraft\" inkscape:label=\"Aircraft\">\n";
    for (const auto& ac : data.aircraft) {
        const double cx    = wx(ac.centerX);
        const double cy    = wy(ac.centerY);
        const double hw    = ac.wingspanFt / 2.0 * scale;
        const double hl    = ac.lengthFt   / 2.0 * scale;
        const double angle = static_cast<double>(ac.rotationDeg);
        const std::string dtStr = arld::core::displayTypeToString(ac.displayType);
        const char* fillColor   = displayTypeColor(dtStr);

        // Font size proportional to smallest aircraft dimension, min 6 px.
        const double minDim   = std::min(ac.wingspanFt, ac.lengthFt) * scale;
        const double fontSize = std::max(6.0, minDim * 0.25);

        svg << "    <g transform=\"translate(" << fmt(cx) << "," << fmt(cy)
            << ") rotate(" << fmt(angle) << ")\">\n";

        // Footprint rectangle
        svg << "      <rect"
            << " x=\""      << fmt(-hw) << "\""
            << " y=\""      << fmt(-hl) << "\""
            << " width=\""  << fmt(hw * 2.0) << "\""
            << " height=\"" << fmt(hl * 2.0) << "\""
            << " fill=\""   << fillColor << "\""
            << " fill-opacity=\"0.85\""
            << " stroke=\"#222222\""
            << " stroke-width=\"" << fmt(0.5 * scale) << "\""
            << "/>\n";

        // Label (display name)
        svg << "      <text"
            << " x=\"0\" y=\"0\""
            << " text-anchor=\"middle\""
            << " dominant-baseline=\"middle\""
            << " font-family=\"sans-serif\""
            << " font-size=\"" << fmt(fontSize) << "\""
            << " fill=\"white\""
            << " font-weight=\"bold\""
            << ">"
            << xmlEscape(ac.displayName)
            << "</text>\n";

        svg << "    </g>\n";
    }
    svg << "  </g><!-- aircraft -->\n";

    // -- Annotations layer (title + scale) --
    svg << "  <g id=\"annotations\" inkscape:label=\"Annotations\">\n";
    {
        const double titleSize = std::max(10.0, svgH * 0.025);
        svg << "    <text"
            << " x=\"" << fmt(kPadding * scale * 0.5) << "\""
            << " y=\"" << fmt(titleSize + 4.0) << "\""
            << " font-family=\"sans-serif\""
            << " font-size=\"" << fmt(titleSize) << "\""
            << " font-weight=\"bold\""
            << " fill=\"#222222\""
            << ">"
            << xmlEscape(data.metadata.title)
            << "</text>\n";

        // Scale note
        svg << "    <text"
            << " x=\"" << fmt(kPadding * scale * 0.5) << "\""
            << " y=\"" << fmt(titleSize * 2.0 + 8.0) << "\""
            << " font-family=\"sans-serif\""
            << " font-size=\"" << fmt(titleSize * 0.65) << "\""
            << " fill=\"#555555\""
            << ">1 unit = 1 ft</text>\n";
    }
    svg << "  </g><!-- annotations -->\n";

    svg << "</svg>\n";

    // ------------------------------------------------------------------
    // 4. Write to file.
    // ------------------------------------------------------------------
    std::ofstream ofs(outputPath);
    if (!ofs.is_open())
        throw std::runtime_error("SvgExporter: cannot open output file: " + outputPath);
    ofs << svg.str();
    if (!ofs.good())
        throw std::runtime_error("SvgExporter: write error: " + outputPath);
}

} // namespace arld::export_
