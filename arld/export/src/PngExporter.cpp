// NOTE: No Qt headers — pure C++20 + standard library only.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <arld/export/PngExporter.h>
#include <arld/core/ProjectFile.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace arld::export_ {

namespace {

struct Pixel { uint8_t r, g, b, a; };
using PixBuffer = std::vector<Pixel>;

// ---- Display-type color ----------------------------------------------------
static Pixel displayTypePixel(const std::string& dt) {
    if (dt == "static_display")        return {0x44, 0x77, 0xAA, 0xD8};
    if (dt == "warbird_heritage")      return {0x44, 0x77, 0x44, 0xD8};
    if (dt == "taxi_only")             return {0xAA, 0x77, 0x44, 0xD8};
    if (dt == "military_static")       return {0x44, 0x55, 0x66, 0xD8};
    if (dt == "hot_ramp")              return {0xAA, 0x44, 0x33, 0xD8};
    if (dt == "media_photo_platform")  return {0x77, 0x44, 0x77, 0xD8};
    if (dt == "ramp_show")             return {0x44, 0x77, 0x88, 0xD8};
    return {0x88, 0x88, 0x88, 0xD8};
}

// ---- Scanline polygon fill -------------------------------------------------
static void fillPolygon(PixBuffer& buf, int W, int H,
                        const std::vector<std::pair<float, float>>& verts,
                        Pixel color) {
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
// PngExporter::exportLayout
// ---------------------------------------------------------------------------
void PngExporter::exportLayout(const arld::core::ProjectData& data,
                               const std::string& outputPath,
                               const ExportOptions& options) {
    constexpr double kPadding = 100.0;   // feet of margin
    constexpr int    kMaxDim  = 16384;   // pixel cap

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
    // 2. Pixel dimensions
    // ------------------------------------------------------------------
    double pixPerFt = options.dpi / 96.0;

    // Cap so neither dimension exceeds kMaxDim
    {
        double wPx = worldW * pixPerFt;
        double hPx = worldH * pixPerFt;
        if (wPx > kMaxDim || hPx > kMaxDim) {
            pixPerFt = (double)(kMaxDim - 1) / std::max(worldW, worldH);
        }
    }

    const int width  = std::max(1, std::min(kMaxDim, (int)std::floor(worldW * pixPerFt)));
    const int height = std::max(1, std::min(kMaxDim, (int)std::floor(worldH * pixPerFt)));

    // World-ft → pixel helpers
    auto wx = [&](double x) -> float { return (float)((x - minX) * pixPerFt); };
    auto wy = [&](double y) -> float { return (float)((y - minY) * pixPerFt); };

    // ------------------------------------------------------------------
    // 3. Allocate buffer (#F8F8F0 background, fully opaque)
    // ------------------------------------------------------------------
    PixBuffer buf((size_t)width * height, {0xF8, 0xF8, 0xF0, 0xFF});

    // ------------------------------------------------------------------
    // 4. Draw boundary
    // ------------------------------------------------------------------
    if (!data.boundary.vertices.empty()) {
        std::vector<std::pair<float, float>> bverts;
        bverts.reserve(data.boundary.vertices.size());
        for (const auto& [x, y] : data.boundary.vertices)
            bverts.push_back({wx(x), wy(y)});
        fillPolygon(buf, width, height, bverts, {0xE8, 0xF5, 0xE8, 0xFF});
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
        Pixel fillColor = displayTypePixel(dtStr);
        fillColor.a = 0xFF; // fully opaque for raster

        auto verts = rotatedRect(cx, cy, hw, hl, ac.rotationDeg);
        fillPolygon(buf, width, height, verts, fillColor);
    }

    // ------------------------------------------------------------------
    // 6. Draw scale bar if requested
    // ------------------------------------------------------------------
    if (options.showScaleBar && width > 40 && height > 20) {
        // Scale bar = 100 ft wide in pixels, capped at width/3
        const int barWidthPx = std::min(static_cast<int>(100.0 * pixPerFt),
                                        width / 3);
        if (barWidthPx >= 4) {
            const int margin = 20;
            const int barHeight = 4;
            const int tickHeight = 10;
            const int barY = height - margin - tickHeight;
            const int barX = margin;

            auto setPixel = [&](int x, int y, Pixel px) {
                if (x < 0 || x >= width || y < 0 || y >= height) return;
                buf[(size_t)y * width + x] = px;
            };

            const Pixel white  = {0xFF, 0xFF, 0xFF, 0xFF};
            const Pixel black  = {0x00, 0x00, 0x00, 0xFF};

            // White filled rectangle (bar body)
            for (int y = barY; y < barY + barHeight; ++y)
                for (int x = barX; x <= barX + barWidthPx; ++x)
                    setPixel(x, y, white);

            // Black border on bar
            for (int x = barX; x <= barX + barWidthPx; ++x) {
                setPixel(x, barY,             black);
                setPixel(x, barY + barHeight - 1, black);
            }
            for (int y = barY; y < barY + barHeight; ++y) {
                setPixel(barX,                y, black);
                setPixel(barX + barWidthPx,   y, black);
            }

            // End ticks (vertical lines above/below the bar)
            for (int y = barY - (tickHeight - barHeight) / 2;
                 y < barY + tickHeight; ++y) {
                setPixel(barX,              y, black);
                setPixel(barX + barWidthPx, y, black);
            }
        }
    }

    // ------------------------------------------------------------------
    // 7. Write PNG
    // ------------------------------------------------------------------
    if (!stbi_write_png(outputPath.c_str(), width, height, 4,
                        buf.data(), width * 4))
        throw std::runtime_error("PngExporter: stbi_write_png failed: " + outputPath);
}

} // namespace arld::export_
