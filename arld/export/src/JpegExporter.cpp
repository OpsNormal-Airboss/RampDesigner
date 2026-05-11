// NOTE: No Qt headers — pure C++20 + standard library only.
// STB_IMAGE_WRITE_IMPLEMENTATION is defined in PngExporter.cpp — do NOT define it here.
#include <stb_image_write.h>
#include <arld/export/JpegExporter.h>
#include <arld/core/ProjectFile.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace arld::export_ {

namespace {

struct RgbPixel { uint8_t r, g, b; };
using RgbBuffer = std::vector<RgbPixel>;

// ---- Display-type color ----------------------------------------------------
static RgbPixel displayTypeRgb(const std::string& dt) {
    if (dt == "static_display")        return {0x44, 0x77, 0xAA};
    if (dt == "warbird_heritage")      return {0x44, 0x77, 0x44};
    if (dt == "taxi_only")             return {0xAA, 0x77, 0x44};
    if (dt == "military_static")       return {0x44, 0x55, 0x66};
    if (dt == "hot_ramp")              return {0xAA, 0x44, 0x33};
    if (dt == "media_photo_platform")  return {0x77, 0x44, 0x77};
    if (dt == "ramp_show")             return {0x44, 0x77, 0x88};
    return {0x88, 0x88, 0x88};
}

// ---- Scanline polygon fill -------------------------------------------------
static void fillPolygon(RgbBuffer& buf, int W, int H,
                        const std::vector<std::pair<float, float>>& verts,
                        RgbPixel color) {
    if (verts.size() < 3) return;
    int ymin = H, ymax = 0;
    for (auto& v : verts) {
        ymin = std::min(ymin, (int)std::floor(v.second));
        ymax = std::max(ymax, (int)std::ceil(v.second));
    }
    ymin = std::max(0, ymin);
    ymax = std::min(H - 1, ymax);
    const int n = (int)verts.size();
    for (int y = ymin; y <= ymax; ++y) {
        std::vector<float> xs;
        for (int i = 0; i < n; ++i) {
            auto [x0, y0] = verts[i];
            auto [x1, y1] = verts[(i + 1) % n];
            if ((y0 <= (float)y && (float)y < y1) ||
                (y1 <= (float)y && (float)y < y0)) {
                float x = x0 + ((float)y - y0) * (x1 - x0) / (y1 - y0);
                xs.push_back(x);
            }
        }
        std::sort(xs.begin(), xs.end());
        for (int k = 0; k + 1 < (int)xs.size(); k += 2) {
            int xa = std::max(0, (int)std::ceil(xs[k]));
            int xb = std::min(W - 1, (int)std::floor(xs[k + 1]));
            for (int x = xa; x <= xb; ++x)
                buf[(size_t)y * W + x] = color;
        }
    }
}

// ---- Rotated rectangle corners ---------------------------------------------
static std::vector<std::pair<float, float>> rotatedRect(
    float cx, float cy, float hw, float hl, float angleDeg) {
    const float rad  = angleDeg * 3.14159265358979f / 180.0f;
    const float cosA = std::cos(rad);
    const float sinA = std::sin(rad);
    float dx[4] = {-hw, +hw, +hw, -hw};
    float dy[4] = {-hl, -hl, +hl, +hl};
    std::vector<std::pair<float, float>> verts(4);
    for (int i = 0; i < 4; ++i) {
        verts[i].first  = cx + dx[i] * cosA - dy[i] * sinA;
        verts[i].second = cy + dx[i] * sinA + dy[i] * cosA;
    }
    return verts;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// JpegExporter::exportLayout
// ---------------------------------------------------------------------------
void JpegExporter::exportLayout(const arld::core::ProjectData& data,
                                const std::string& outputPath,
                                const ExportOptions& options) {
    constexpr double kPadding = 100.0;   // feet of margin
    constexpr int    kMaxDim  = 32767;   // JPEG hard limit

    // ------------------------------------------------------------------
    // 1. Bounding box (same logic as SvgExporter)
    // ------------------------------------------------------------------
    double minX = DBL_MAX, minY = DBL_MAX, maxX = -DBL_MAX, maxY = -DBL_MAX;

    auto expand = [&](double x, double y) {
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    };

    for (const auto& [x, y] : data.boundary.vertices)
        expand(x, y);

    for (const auto& ac : data.aircraft) {
        const double hw     = ac.wingspanFt / 2.0;
        const double hl     = ac.lengthFt   / 2.0;
        const double radius = std::sqrt(hw * hw + hl * hl);
        expand(ac.centerX - radius, ac.centerY - radius);
        expand(ac.centerX + radius, ac.centerY + radius);
    }

    if (minX == DBL_MAX) {
        minX = -100.0; minY = -100.0;
        maxX =  100.0; maxY =  100.0;
    }

    minX -= kPadding; minY -= kPadding;
    maxX += kPadding; maxY += kPadding;

    const double worldW = maxX - minX;
    const double worldH = maxY - minY;

    // ------------------------------------------------------------------
    // 2. Pixel dimensions — cap at kMaxDim
    // ------------------------------------------------------------------
    double pixPerFt = options.dpi / 96.0;

    double wPx = worldW * pixPerFt;
    double hPx = worldH * pixPerFt;
    if (wPx > kMaxDim || hPx > kMaxDim) {
        std::fprintf(stderr, "JpegExporter: image dimension capped to 32767px\n");
        // Recompute pixPerFt so largest dimension is exactly kMaxDim pixels.
        pixPerFt = (double)(kMaxDim - 1) / std::max(worldW, worldH);
    }

    // Use floor to guarantee we stay within the kMaxDim cap after rounding.
    const int width  = std::max(1, std::min(kMaxDim, (int)std::floor(worldW * pixPerFt)));
    const int height = std::max(1, std::min(kMaxDim, (int)std::floor(worldH * pixPerFt)));

    // World-ft → pixel helpers
    auto wx = [&](double x) -> float { return (float)((x - minX) * pixPerFt); };
    auto wy = [&](double y) -> float { return (float)((y - minY) * pixPerFt); };

    // ------------------------------------------------------------------
    // 3. Allocate buffer (#F8F8F0 background, RGB)
    // ------------------------------------------------------------------
    RgbBuffer buf((size_t)width * height, {0xF8, 0xF8, 0xF0});

    // ------------------------------------------------------------------
    // 4. Draw boundary
    // ------------------------------------------------------------------
    if (!data.boundary.vertices.empty()) {
        std::vector<std::pair<float, float>> bverts;
        bverts.reserve(data.boundary.vertices.size());
        for (const auto& [x, y] : data.boundary.vertices)
            bverts.push_back({wx(x), wy(y)});
        fillPolygon(buf, width, height, bverts, {0xE8, 0xF5, 0xE8});
    }

    // ------------------------------------------------------------------
    // 5. Draw aircraft
    // ------------------------------------------------------------------
    for (const auto& ac : data.aircraft) {
        const float cx    = wx(ac.centerX);
        const float cy    = wy(ac.centerY);
        const float hw    = (float)(ac.wingspanFt / 2.0 * pixPerFt);
        const float hl    = (float)(ac.lengthFt   / 2.0 * pixPerFt);
        const std::string dtStr = arld::core::displayTypeToString(ac.displayType);
        RgbPixel fillColor = displayTypeRgb(dtStr);

        auto verts = rotatedRect(cx, cy, hw, hl, ac.rotationDeg);
        fillPolygon(buf, width, height, verts, fillColor);
    }

    // ------------------------------------------------------------------
    // 6. Write JPEG
    // ------------------------------------------------------------------
    const int quality = std::max(1, std::min(100, options.jpegQuality));
    if (!stbi_write_jpg(outputPath.c_str(), width, height, 3,
                        buf.data(), quality))
        throw std::runtime_error("JpegExporter: stbi_write_jpg failed: " + outputPath);
}

} // namespace arld::export_
