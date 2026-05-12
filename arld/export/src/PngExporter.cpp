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

// ---- Embedded 5x7 bitmap font (ASCII subset) --------------------------------
static const uint8_t* fontGlyph(char c) {
    static const uint8_t G[][7] = {
        /* ' ' */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        /* '.' */ {0x00,0x00,0x00,0x00,0x00,0x60,0x00},
        /* '/' */ {0x08,0x10,0x10,0x20,0x20,0x40,0x40},
        /* '0' */ {0x70,0x88,0x98,0xA8,0xC8,0x88,0x70},
        /* '1' */ {0x20,0x60,0x20,0x20,0x20,0x20,0x70},
        /* '2' */ {0x70,0x88,0x08,0x30,0x40,0x80,0xF8},
        /* '3' */ {0x70,0x88,0x08,0x30,0x08,0x88,0x70},
        /* '4' */ {0x10,0x30,0x50,0x90,0xF8,0x10,0x10},
        /* '5' */ {0xF8,0x80,0xF0,0x08,0x08,0x88,0x70},
        /* '6' */ {0x38,0x40,0x80,0xF0,0x88,0x88,0x70},
        /* '7' */ {0xF8,0x08,0x10,0x20,0x20,0x40,0x40},
        /* '8' */ {0x70,0x88,0x88,0x70,0x88,0x88,0x70},
        /* '9' */ {0x70,0x88,0x88,0x78,0x08,0x10,0x60},
        /* 'f' */ {0x18,0x20,0x70,0x20,0x20,0x20,0x20},
        /* 'm' */ {0x00,0x00,0xD8,0xA8,0xA8,0xA8,0xA8},
        /* 't' */ {0x20,0x20,0x70,0x20,0x20,0x20,0x18},
    };
    switch(c) {
        case ' ': return G[0];
        case '.': return G[1];
        case '/': return G[2];
        case '0': return G[3];
        case '1': return G[4];
        case '2': return G[5];
        case '3': return G[6];
        case '4': return G[7];
        case '5': return G[8];
        case '6': return G[9];
        case '7': return G[10];
        case '8': return G[11];
        case '9': return G[12];
        case 'f': return G[13];
        case 'm': return G[14];
        case 't': return G[15];
        default:  return G[0];
    }
}

static void drawLabel(PixBuffer& buf, int W, int H, int x, int y,
                      const std::string& text, Pixel color, int scale = 1) {
    int cx = x;
    for (char c : text) {
        const uint8_t* glyph = fontGlyph(c);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph[row] & (0x80 >> col)) {
                    for (int sy = 0; sy < scale; ++sy)
                        for (int sx = 0; sx < scale; ++sx) {
                            int px = cx + col * scale + sx;
                            int py = y  + row * scale + sy;
                            if (px >= 0 && px < W && py >= 0 && py < H)
                                buf[(size_t)py * W + px] = color;
                        }
                }
            }
        }
        cx += 6 * scale;
    }
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
    if (options.showScaleBar && width > 60 && height > 30) {
        const int targetBarPx = width / 4;

        int barFt = 0;
        int barPx = 0;
        std::string label;

        if (options.scaleBarMode == ExportOptions::ScaleBarMode::MetricOnly) {
            static const int kMCandidates[] = {5, 10, 25, 50, 100, 250, 500, 1000, 2500};
            int bestM = kMCandidates[0];
            double bestDiff = DBL_MAX;
            for (int m : kMCandidates) {
                double px = (m / 0.3048) * pixPerFt;
                if (std::abs(px - targetBarPx) < bestDiff) {
                    bestDiff = std::abs(px - targetBarPx);
                    bestM = m;
                }
            }
            barPx = static_cast<int>((bestM / 0.3048) * pixPerFt);
            label = std::to_string(bestM) + " m";
        } else {
            static const int kFCandidates[] = {10, 25, 50, 100, 250, 500, 1000, 2500, 5000};
            int bestFt = kFCandidates[0];
            double bestDiff = DBL_MAX;
            for (int ft : kFCandidates) {
                double px = ft * pixPerFt;
                if (std::abs(px - targetBarPx) < bestDiff) {
                    bestDiff = std::abs(px - targetBarPx);
                    bestFt = ft;
                }
            }
            barFt = bestFt;
            barPx = static_cast<int>(barFt * pixPerFt);
            if (options.scaleBarMode == ExportOptions::ScaleBarMode::Dual) {
                int nearestM = static_cast<int>(std::round(barFt * 0.3048 / 10.0) * 10);
                if (nearestM < 1) nearestM = 1;
                label = std::to_string(barFt) + " ft / " + std::to_string(nearestM) + " m";
            } else {
                label = std::to_string(barFt) + " ft";
            }
        }

        barPx = std::min(barPx, width / 3);
        if (barPx >= 4) {
            const int margin  = 20;
            const int barH    = 4;
            const int tickH   = 10;
            const int barY    = height - margin - tickH;
            const int barX    = margin;
            const Pixel white = {0xFF,0xFF,0xFF,0xFF};
            const Pixel black = {0x00,0x00,0x00,0xFF};

            auto setPixel = [&](int x, int y, Pixel px) {
                if (x >= 0 && x < width && y >= 0 && y < height)
                    buf[(size_t)y * width + x] = px;
            };

            for (int y = barY; y < barY + barH; ++y)
                for (int x = barX; x <= barX + barPx; ++x)
                    setPixel(x, y, white);
            for (int x = barX; x <= barX + barPx; ++x) {
                setPixel(x, barY,             black);
                setPixel(x, barY + barH - 1,  black);
            }
            for (int y = barY; y < barY + barH; ++y) {
                setPixel(barX,         y, black);
                setPixel(barX + barPx, y, black);
            }
            for (int y = barY - (tickH - barH) / 2; y < barY + tickH; ++y) {
                setPixel(barX,         y, black);
                setPixel(barX + barPx, y, black);
            }
            const int textScale = (width >= 800) ? 2 : 1;
            const int textY = barY - (tickH - barH) / 2 - 7 * textScale - 2;
            drawLabel(buf, width, height, barX, textY, label, black, textScale);
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
