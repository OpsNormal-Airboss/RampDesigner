// NOTE: No Qt headers — pure C++20 + standard library only.
#include <arld/export/PdfExporter.h>
#include <arld/core/ProjectFile.h>

#ifdef HAVE_LIBHARU
#  include <hpdf.h>
#  ifdef HAVE_QRENCODE
#    include <qrencode.h>
#  endif
#endif

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace arld::export_ {

// ---------------------------------------------------------------------------
// RGB -> CMYK conversion (simple approximation, no ICC profile)
// ---------------------------------------------------------------------------
static void rgbToCmyk(float r, float g, float b,
                      float& c, float& m, float& y, float& k) {
    k = 1.0f - std::max({r, g, b});
    if (k < 1.0f) {
        c = (1.0f - r - k) / (1.0f - k);
        m = (1.0f - g - k) / (1.0f - k);
        y = (1.0f - b - k) / (1.0f - k);
    } else {
        c = m = y = 0.0f;
    }
}

// Parse a hex color string like "#4477AA" to RGB floats [0,1].
static bool parseHexColor(const char* hex, float& r, float& g, float& b) {
    if (!hex || hex[0] != '#') return false;
    unsigned int ri = 0, gi = 0, bi = 0;
    if (sscanf(hex + 1, "%02x%02x%02x", &ri, &gi, &bi) != 3) return false;
    r = static_cast<float>(ri) / 255.0f;
    g = static_cast<float>(gi) / 255.0f;
    b = static_cast<float>(bi) / 255.0f;
    return true;
}

// Display-type color hex strings (same palette as SvgExporter).
static const char* displayTypeColor(const std::string& dt) {
    if (dt == "static_display")       return "#4477AA";
    if (dt == "warbird_heritage")     return "#447744";
    if (dt == "taxi_only")            return "#AA7744";
    if (dt == "military_static")      return "#445566";
    if (dt == "hot_ramp")             return "#AA4433";
    if (dt == "media_photo_platform") return "#774477";
    if (dt == "ramp_show")            return "#447788";
    return "#888888";
}

// ---------------------------------------------------------------------------
// Compact SHA-256 (RFC 6234) — needed for QR code content
// ---------------------------------------------------------------------------
namespace sha256_impl {

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static inline uint32_t rotr32(uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }

static std::string compute(const uint8_t* data, size_t len) {
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // Pad the message.
    size_t bitLen = len * 8;
    size_t padLen = ((len + 8) % 64 <= 55)
                    ? (55 - (len + 8) % 64 + 1 + 8)
                    : (64 - (len + 8) % 64 + 55 - 7 + 8);
    // simpler: total blocks
    size_t totalLen = len + 1 + padLen; // >= len+9, padded to multiple of 64

    // Recalculate the simple way
    size_t padded = len + 1;
    while (padded % 64 != 56) padded++;
    padded += 8; // for length

    std::vector<uint8_t> msg(padded, 0);
    memcpy(msg.data(), data, len);
    msg[len] = 0x80;
    // Append bit length big-endian in last 8 bytes
    uint64_t bitLenBE = static_cast<uint64_t>(len) * 8;
    for (int i = 7; i >= 0; --i) {
        msg[padded - 8 + (7 - i)] = static_cast<uint8_t>((bitLenBE >> (i * 8)) & 0xFF);
    }

    for (size_t blk = 0; blk < padded / 64; ++blk) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(msg[blk * 64 + i * 4    ]) << 24)
                 | (static_cast<uint32_t>(msg[blk * 64 + i * 4 + 1]) << 16)
                 | (static_cast<uint32_t>(msg[blk * 64 + i * 4 + 2]) <<  8)
                 | (static_cast<uint32_t>(msg[blk * 64 + i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >> 3);
            uint32_t s1 = rotr32(w[i-2],  17) ^ rotr32(w[i-2],  19) ^ (w[i-2]  >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t S1  = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            uint32_t ch  = (e & f) ^ (~e & g);
            uint32_t tmp1 = hh + S1 + ch + K[i] + w[i];
            uint32_t S0  = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t tmp2 = S0 + maj;
            hh = g; g = f; f = e; e = d + tmp1;
            d = c; c = b; b = a; a = tmp1 + tmp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    char hex[65];
    snprintf(hex, sizeof(hex),
             "%08x%08x%08x%08x%08x%08x%08x%08x",
             h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]);
    return std::string(hex);
}

} // namespace sha256_impl

// ---------------------------------------------------------------------------
// BBox helper (same as SvgExporter)
// ---------------------------------------------------------------------------
struct PdfBBox {
    double minX = DBL_MAX, minY = DBL_MAX, maxX = -DBL_MAX, maxY = -DBL_MAX;
    void expand(double x, double y) {
        minX = std::min(minX, x); minY = std::min(minY, y);
        maxX = std::max(maxX, x); maxY = std::max(maxY, y);
    }
    bool empty() const { return minX == DBL_MAX; }
};

#ifdef HAVE_LIBHARU

// ---------------------------------------------------------------------------
// libharu error handler
// ---------------------------------------------------------------------------
static void hpdfErrorHandler(HPDF_STATUS errorNo, HPDF_STATUS detailNo, void*) {
    throw std::runtime_error("libharu error: " + std::to_string(errorNo)
                             + " detail: " + std::to_string(detailNo));
}

// ---------------------------------------------------------------------------
// Paper sizes in points (1 pt = 1/72 inch). Width x Height (portrait).
// ---------------------------------------------------------------------------
static std::pair<float, float> paperSizePts(ExportOptions::PaperSize ps) {
    switch (ps) {
        case ExportOptions::PaperSize::Letter:  return {612.0f,  792.0f};
        case ExportOptions::PaperSize::Tabloid: return {792.0f, 1224.0f};
        case ExportOptions::PaperSize::ANSI_C:  return {1224.0f, 1584.0f};
        case ExportOptions::PaperSize::ANSI_D:  return {1584.0f, 2448.0f};
        case ExportOptions::PaperSize::ANSI_E:  return {2448.0f, 3168.0f};
        case ExportOptions::PaperSize::ANSI_E1: return {2160.0f, 3024.0f};
    }
    return {1584.0f, 2448.0f}; // ANSI-D default
}

// Draw text helper (shorthand).
static void drawText(HPDF_Page page, HPDF_Font font, float fontSize,
                     float x, float y, const std::string& text) {
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, fontSize);
    HPDF_Page_TextOut(page, x, y, text.c_str());
    HPDF_Page_EndText(page);
}

// Draw a filled rectangle using CMYK colors.
static void fillRectCmyk(HPDF_Page page, float x, float y, float w, float h,
                          float c, float m, float yw, float k) {
    HPDF_Page_SetCMYKFill(page, c, m, yw, k);
    HPDF_Page_Rectangle(page, x, y, w, h);
    HPDF_Page_Fill(page);
}

// Draw a stroked rectangle using CMYK colors.
static void strokeRectCmyk(HPDF_Page page, float x, float y, float w, float h,
                            float c, float m, float yw, float k, float lineWidth) {
    HPDF_Page_SetCMYKStroke(page, c, m, yw, k);
    HPDF_Page_SetLineWidth(page, lineWidth);
    HPDF_Page_Rectangle(page, x, y, w, h);
    HPDF_Page_Stroke(page);
}

// Severity string for the violations table.
static const char* severityLabel(arld::core::ClearanceSeverity sev) {
    switch (sev) {
        case arld::core::ClearanceSeverity::Advisory:   return "Advisory";
        case arld::core::ClearanceSeverity::Violation:  return "Violation";
        case arld::core::ClearanceSeverity::Overridden: return "Overridden";
        case arld::core::ClearanceSeverity::Clear:      return "Clear";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// Draw QR code into the page at (qrX, qrY) with the given box size.
// Returns true on success.
// ---------------------------------------------------------------------------
#ifdef HAVE_QRENCODE
static bool drawQrCode(HPDF_Page page, const std::string& text,
                       float qrX, float qrY, float boxSize) {
    QRcode* qr = QRcode_encodeString(text.c_str(), 0, QR_ECLEVEL_M,
                                     QR_MODE_8, 1);
    if (!qr) return false;

    const int width = qr->width;
    const float moduleSize = boxSize / static_cast<float>(width);

    for (int row = 0; row < width; ++row) {
        for (int col = 0; col < width; ++col) {
            if (qr->data[row * width + col] & 0x01) {
                // QR origin is top-left; PDF origin is bottom-left.
                float x = qrX + col * moduleSize;
                float y = qrY + (width - 1 - row) * moduleSize;
                HPDF_Page_SetCMYKFill(page, 0.0f, 0.0f, 0.0f, 1.0f);
                HPDF_Page_Rectangle(page, x, y, moduleSize, moduleSize);
                HPDF_Page_Fill(page);
            }
        }
    }
    QRcode_free(qr);
    return true;
}
#endif // HAVE_QRENCODE

// ---------------------------------------------------------------------------
// Full PDF export (libharu path)
// ---------------------------------------------------------------------------
static void exportWithLibharu(const arld::core::ProjectData& data,
                               const std::string& outputPath,
                               const ExportOptions& options) {
    HPDF_Doc pdf = HPDF_New(hpdfErrorHandler, nullptr);
    if (!pdf)
        throw std::runtime_error("PdfExporter: HPDF_New failed");

    // Use UTF-8 encoding
    HPDF_UseUTFEncodings(pdf);

    // --- Font selection ---
    HPDF_Font font = nullptr;
    if (!options.fontPath.empty()) {
        // Try to load TrueType font
        std::ifstream test(options.fontPath);
        if (test.good()) {
            const char* fontName = HPDF_LoadTTFontFromFile(pdf, options.fontPath.c_str(), HPDF_TRUE);
            if (fontName) {
                font = HPDF_GetFont(pdf, fontName, "UTF-8");
            }
        }
    }
    if (!font) {
        font = HPDF_GetFont(pdf, "Helvetica", nullptr);
    }

    // --- Paper dimensions ---
    auto [pw, ph] = paperSizePts(options.paperSize);
    if (options.orientation == ExportOptions::Orientation::Landscape)
        std::swap(pw, ph);

    constexpr float kTitleBlockH = 108.0f; // 1.5 inches

    // --- Bounding box computation ---
    PdfBBox bbox;
    for (const auto& [x, y] : data.boundary.vertices)
        bbox.expand(x, y);
    for (const auto& ac : data.aircraft) {
        const double hw = ac.wingspanFt / 2.0;
        const double hl = ac.lengthFt / 2.0;
        const double r  = std::sqrt(hw * hw + hl * hl);
        bbox.expand(ac.centerX - r, ac.centerY - r);
        bbox.expand(ac.centerX + r, ac.centerY + r);
    }
    if (bbox.empty()) bbox = {-100.0, -100.0, 100.0, 100.0};

    constexpr double kPadFrac = 0.05;
    const double worldW = bbox.maxX - bbox.minX;
    const double worldH = bbox.maxY - bbox.minY;
    const double padX   = worldW * kPadFrac;
    const double padY   = worldH * kPadFrac;
    bbox.minX -= padX; bbox.minY -= padY;
    bbox.maxX += padX; bbox.maxY += padY;

    const double bboxW = bbox.maxX - bbox.minX;
    const double bboxH = bbox.maxY - bbox.minY;

    // Content area (above title block)
    const float contentW = pw;
    const float contentH = ph - kTitleBlockH;

    // Scale to fit content area
    const double scaleX = (bboxW > 0) ? (contentW / bboxW) : 1.0;
    const double scaleY = (bboxH > 0) ? (contentH / bboxH) : 1.0;
    const double scale  = std::min(scaleX, scaleY);

    // Centering offsets
    const double drawW = bboxW * scale;
    const double drawH = bboxH * scale;
    const float  offX  = static_cast<float>((contentW  - drawW) / 2.0);
    const float  offY  = static_cast<float>((contentH - drawH) / 2.0) + kTitleBlockH;

    auto ftToX = [&](double x) -> float {
        return static_cast<float>((x - bbox.minX) * scale) + offX;
    };
    auto ftToY = [&](double y) -> float {
        // PDF y increases upward; scene y increases downward.
        return offY + static_cast<float>((bbox.maxY - y) * scale);
    };

    // -----------------------------------------------------------------------
    // Page 1: Layout drawing
    // -----------------------------------------------------------------------
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetWidth(page,  pw);
    HPDF_Page_SetHeight(page, ph);

    // White background
    HPDF_Page_SetCMYKFill(page, 0.0f, 0.0f, 0.0f, 0.0f);
    HPDF_Page_Rectangle(page, 0, 0, pw, ph);
    HPDF_Page_Fill(page);

    // --- Ramp boundary polygon ---
    if (!data.boundary.vertices.empty()) {
        // Light green fill
        float fr, fg, fb, fc, fm, fy, fk;
        parseHexColor("#E8F5E8", fr, fg, fb);
        rgbToCmyk(fr, fg, fb, fc, fm, fy, fk);
        HPDF_Page_SetCMYKFill(page, fc, fm, fy, fk);

        float sr, sg, sb, sc, sm, sy, sk;
        parseHexColor("#338833", sr, sg, sb);
        rgbToCmyk(sr, sg, sb, sc, sm, sy, sk);
        HPDF_Page_SetCMYKStroke(page, sc, sm, sy, sk);
        HPDF_Page_SetLineWidth(page, 1.5f);

        bool first = true;
        for (const auto& [x, y] : data.boundary.vertices) {
            float px = ftToX(x);
            float py = ftToY(y);
            if (first) { HPDF_Page_MoveTo(page, px, py); first = false; }
            else        HPDF_Page_LineTo(page, px, py);
        }
        HPDF_Page_ClosePathFillStroke(page);
    }

    // --- Aircraft rectangles ---
    for (const auto& ac : data.aircraft) {
        const float cx = ftToX(ac.centerX);
        const float cy = ftToY(ac.centerY);
        const float hw = static_cast<float>(ac.wingspanFt / 2.0 * scale);
        const float hl = static_cast<float>(ac.lengthFt   / 2.0 * scale);
        const float angle = static_cast<float>(ac.rotationDeg);
        const std::string dtStr = arld::core::displayTypeToString(ac.displayType);
        const char* hexColor = displayTypeColor(dtStr);

        float r, g, b, c, m, y, k;
        parseHexColor(hexColor, r, g, b);
        rgbToCmyk(r, g, b, c, m, y, k);

        // Save graphics state, translate and rotate, draw rect, restore.
        HPDF_Page_GSave(page);
        HPDF_Page_Concat(page, 1.0f, 0.0f, 0.0f, 1.0f, cx, cy);
        // PDF rotation is counter-clockwise in radians; Qt is CW in degrees.
        float rad = -angle * static_cast<float>(std::numbers::pi) / 180.0f;
        HPDF_Page_Concat(page,
            std::cos(rad), std::sin(rad),
            -std::sin(rad), std::cos(rad),
            0.0f, 0.0f);

        // Filled rectangle (origin at center)
        HPDF_Page_SetCMYKFill(page, c, m, y, k);
        HPDF_Page_SetCMYKStroke(page, 0.0f, 0.0f, 0.0f, 1.0f);
        HPDF_Page_SetLineWidth(page, 0.5f);
        HPDF_Page_Rectangle(page, -hw, -hl, hw * 2.0f, hl * 2.0f);
        HPDF_Page_FillStroke(page);

        // Label: prefer tail number, fall back to display name
        const std::string label = ac.tailNumber.empty() ? ac.displayName : ac.tailNumber;
        if (!label.empty() && font) {
            const float fontSize = std::max(6.0f, std::min(hw, hl) * 0.4f);
            HPDF_Page_SetCMYKFill(page, 0.0f, 0.0f, 0.0f, 0.0f); // white
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font, fontSize);
            HPDF_Page_TextOut(page, -hw * 0.8f, -fontSize * 0.4f, label.c_str());
            HPDF_Page_EndText(page);
        }

        HPDF_Page_GRestore(page);
    }

    // -----------------------------------------------------------------------
    // Scale bar (lower-left of content area)
    // -----------------------------------------------------------------------
    if (options.showScaleBar && scale > 0.0 && font) {
        static const int kCandidates[] = {10, 25, 50, 100, 250, 500, 1000, 2500, 5000};
        const float targetBarPts = pw * 0.15f;

        int barFt = kCandidates[0];
        float bestDiff = FLT_MAX;
        for (int c : kCandidates) {
            float pts = static_cast<float>(c * scale);
            float diff = std::abs(pts - targetBarPts);
            if (diff < bestDiff) { bestDiff = diff; barFt = c; }
        }

        const float barPts  = static_cast<float>(barFt * scale);
        const float sbX     = std::max(20.0f, offX);
        const float sbY     = kTitleBlockH + 24.0f;
        const float sbH     = 4.0f;
        const float tickH   = 8.0f;
        const float tickOff = (tickH - sbH) / 2.0f;

        HPDF_Page_SetCMYKFill(page, 0.0f, 0.0f, 0.0f, 0.0f);
        HPDF_Page_Rectangle(page, sbX, sbY, barPts, sbH);
        HPDF_Page_Fill(page);

        HPDF_Page_SetCMYKStroke(page, 0.0f, 0.0f, 0.0f, 1.0f);
        HPDF_Page_SetLineWidth(page, 0.5f);
        HPDF_Page_Rectangle(page, sbX, sbY, barPts, sbH);
        HPDF_Page_Stroke(page);

        HPDF_Page_MoveTo(page, sbX,           sbY - tickOff);
        HPDF_Page_LineTo(page, sbX,           sbY + sbH + tickOff);
        HPDF_Page_Stroke(page);
        HPDF_Page_MoveTo(page, sbX + barPts,  sbY - tickOff);
        HPDF_Page_LineTo(page, sbX + barPts,  sbY + sbH + tickOff);
        HPDF_Page_Stroke(page);

        std::string sbLabel;
        if (options.scaleBarMode == ExportOptions::ScaleBarMode::MetricOnly) {
            int nearestM = static_cast<int>(std::round(barFt * 0.3048 / 5.0) * 5);
            if (nearestM < 1) nearestM = 1;
            sbLabel = std::to_string(nearestM) + " m";
        } else if (options.scaleBarMode == ExportOptions::ScaleBarMode::Dual) {
            int nearestM = static_cast<int>(std::round(barFt * 0.3048 / 10.0) * 10);
            if (nearestM < 1) nearestM = 1;
            sbLabel = std::to_string(barFt) + " ft / " + std::to_string(nearestM) + " m";
        } else {
            sbLabel = std::to_string(barFt) + " ft";
        }

        HPDF_Page_SetCMYKFill(page, 0.0f, 0.0f, 0.0f, 1.0f);
        drawText(page, font, 6.0f, sbX, sbY + sbH + tickOff + 2.0f, sbLabel);
    }

    // -----------------------------------------------------------------------
    // Title block (bottom kTitleBlockH pts)
    // -----------------------------------------------------------------------
    constexpr float kMargin = 6.0f;

    // Horizontal rule
    HPDF_Page_SetCMYKStroke(page, 0.0f, 0.0f, 0.0f, 1.0f);
    HPDF_Page_SetLineWidth(page, 1.0f);
    HPDF_Page_MoveTo(page, 0.0f, kTitleBlockH);
    HPDF_Page_LineTo(page, pw,   kTitleBlockH);
    HPDF_Page_Stroke(page);

    // Title block background
    HPDF_Page_SetCMYKFill(page, 0.0f, 0.0f, 0.0f, 0.04f);
    HPDF_Page_Rectangle(page, 0.0f, 0.0f, pw, kTitleBlockH);
    HPDF_Page_Fill(page);

    if (font) {
        HPDF_Page_SetCMYKFill(page, 0.0f, 0.0f, 0.0f, 1.0f);

        // Show name (large bold) — left column
        const std::string showName = options.showName.empty()
                                     ? data.metadata.title
                                     : options.showName;
        const float bigFontSize = 16.0f;
        const float smallFontSize = 8.0f;

        drawText(page, font, bigFontSize,
                 kMargin, kTitleBlockH - bigFontSize - kMargin, showName);

        // Show date
        if (!options.showDate.empty())
            drawText(page, font, smallFontSize,
                     kMargin,
                     kTitleBlockH - bigFontSize - 2.0f * kMargin - smallFontSize,
                     "Date: " + options.showDate);

        // Show venue
        if (!options.showVenue.empty())
            drawText(page, font, smallFontSize,
                     kMargin,
                     kTitleBlockH - bigFontSize - 3.0f * kMargin - 2.0f * smallFontSize,
                     "Venue: " + options.showVenue);

        // Right column: version info
        const float midX = pw * 0.5f;
        drawText(page, font, smallFontSize,
                 midX, kTitleBlockH - smallFontSize - kMargin, "ARLD v1.1.0");
        drawText(page, font, smallFontSize,
                 midX, kTitleBlockH - 2.0f * smallFontSize - 2.0f * kMargin,
                 "Exported: " + arld::core::ProjectFile::currentUtcTimestamp());

        // ----------------------------------------------------------------
        // Display-type legend (bottom-left area)
        // ----------------------------------------------------------------
        struct LegendEntry { const char* hex; const char* label; };
        static const LegendEntry kLegend[] = {
            {"#4477AA", "Static Display"},
            {"#447744", "Warbird / Heritage"},
            {"#AA7744", "Taxi Only"},
            {"#445566", "Military Static"},
            {"#AA4433", "Hot Ramp"},
            {"#774477", "Media / Photo Platform"},
            {"#447788", "Ramp Show"},
        };
        constexpr int kLegendCount = 7;
        const float swatchSize = 8.0f;
        const float legendX    = kMargin;
        const float legendBaseY = swatchSize + kMargin;

        for (int i = 0; i < kLegendCount; ++i) {
            float lx = legendX + static_cast<float>(i) * (swatchSize + 60.0f);
            float ly = legendBaseY;

            float lr, lg, lb, lc, lm, ly2, lk;
            parseHexColor(kLegend[i].hex, lr, lg, lb);
            rgbToCmyk(lr, lg, lb, lc, lm, ly2, lk);
            HPDF_Page_SetCMYKFill(page, lc, lm, ly2, lk);
            HPDF_Page_Rectangle(page, lx, ly, swatchSize, swatchSize);
            HPDF_Page_Fill(page);

            HPDF_Page_SetCMYKFill(page, 0.0f, 0.0f, 0.0f, 1.0f);
            drawText(page, font, 6.0f, lx + swatchSize + 2.0f, ly + 1.0f, kLegend[i].label);
        }
    }

    // ----------------------------------------------------------------
    // QR code (far-right area of title block)
    // ----------------------------------------------------------------
#ifdef HAVE_QRENCODE
    {
        std::string qrContent;
        if (!options.arldFilePath.empty()) {
            // Read file and SHA-256 hash it
            std::ifstream qrFile(options.arldFilePath, std::ios::binary);
            if (qrFile.good()) {
                std::vector<uint8_t> fileBytes(
                    (std::istreambuf_iterator<char>(qrFile)),
                    std::istreambuf_iterator<char>());
                qrContent = "arld:sha256:" + sha256_impl::compute(fileBytes.data(), fileBytes.size());
            }
        }
        if (qrContent.empty()) {
            // Fallback: title + export date
            qrContent = "arld:" + data.metadata.title + ":"
                        + arld::core::ProjectFile::currentUtcTimestamp();
        }

        constexpr float kQrBoxSize = 80.0f;
        const float qrX = pw - kQrBoxSize - kMargin;
        const float qrY = kMargin;
        drawQrCode(page, qrContent, qrX, qrY, kQrBoxSize);
    }
#endif

    // -----------------------------------------------------------------------
    // Page 2: Violations report (if requested)
    // -----------------------------------------------------------------------
    if (options.includeViolations && !options.violations.empty()) {
        HPDF_Page vPage = HPDF_AddPage(pdf);
        HPDF_Page_SetWidth(vPage,  pw);
        HPDF_Page_SetHeight(vPage, ph);

        HPDF_Page_SetCMYKFill(vPage, 0.0f, 0.0f, 0.0f, 0.0f);
        HPDF_Page_Rectangle(vPage, 0, 0, pw, ph);
        HPDF_Page_Fill(vPage);

        if (font) {
            HPDF_Page_SetCMYKFill(vPage, 0.0f, 0.0f, 0.0f, 1.0f);
            const float margin = 36.0f;
            float yPos = ph - margin;

            // Title
            drawText(vPage, font, 16.0f, margin, yPos, "Violations Report");
            yPos -= 28.0f;

            // Column headers
            const float colW = (pw - 2.0f * margin) / 5.0f;
            static const char* headers[] = {
                "Aircraft A", "Aircraft B", "Severity", "Gap (ft)", "Required (ft)"
            };
            for (int c = 0; c < 5; ++c) {
                drawText(vPage, font, 8.0f,
                         margin + c * colW, yPos, headers[c]);
            }
            yPos -= 16.0f;

            // Divider
            HPDF_Page_SetCMYKStroke(vPage, 0.0f, 0.0f, 0.0f, 1.0f);
            HPDF_Page_SetLineWidth(vPage, 0.5f);
            HPDF_Page_MoveTo(vPage, margin,        yPos + 4.0f);
            HPDF_Page_LineTo(vPage, pw - margin,   yPos + 4.0f);
            HPDF_Page_Stroke(vPage);

            // Rows
            for (const auto& v : options.violations) {
                if (yPos < margin + 20.0f) break; // safety: don't overflow page
                char gapBuf[16], reqBuf[16];
                snprintf(gapBuf, sizeof(gapBuf), "%.1f", static_cast<double>(v.separationFt));
                snprintf(reqBuf, sizeof(reqBuf), "%.1f", static_cast<double>(v.requiredFt));
                const std::string row[] = {
                    v.idA, v.idB, severityLabel(v.severity), gapBuf, reqBuf
                };
                for (int c = 0; c < 5; ++c) {
                    drawText(vPage, font, 7.0f, margin + c * colW, yPos, row[c]);
                }
                yPos -= 12.0f;
            }

            // Overrides section
            if (!options.overrides.empty()) {
                yPos -= 16.0f;
                drawText(vPage, font, 12.0f, margin, yPos, "Override Justifications");
                yPos -= 20.0f;
                for (const auto& ov : options.overrides) {
                    if (yPos < margin + 20.0f) break;
                    const std::string line = ov.placementIdA + " / " + ov.placementIdB
                                             + ": " + ov.justification;
                    drawText(vPage, font, 7.0f, margin, yPos, line);
                    yPos -= 12.0f;
                }
            }
        }
    }

    // --- Write to file ---
    if (HPDF_SaveToFile(pdf, outputPath.c_str()) != HPDF_OK) {
        HPDF_Free(pdf);
        throw std::runtime_error("PdfExporter: HPDF_SaveToFile failed: " + outputPath);
    }
    HPDF_Free(pdf);
}

#endif // HAVE_LIBHARU

// ---------------------------------------------------------------------------
// PdfExporter::exportLayout
// ---------------------------------------------------------------------------
void PdfExporter::exportLayout(const arld::core::ProjectData& data,
                               const std::string& outputPath,
                               const ExportOptions& options) {
#ifdef HAVE_LIBHARU
    exportWithLibharu(data, outputPath, options);
#else
    // Stub: write an empty file so tests can still check path creation.
    (void)data; (void)options;
    std::ofstream f(outputPath);
    if (!f.is_open())
        throw std::runtime_error("PdfExporter: cannot open output file: " + outputPath);
#endif
}

} // namespace arld::export_
