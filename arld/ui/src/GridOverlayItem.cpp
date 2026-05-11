#include <arld/ui/GridOverlayItem.h>
#include <QPainter>
#include <QPen>
#include <cmath>

namespace arld::ui {

static constexpr double kSceneBounds = 50000.0; // scene extends ±50000 ft

GridOverlayItem::GridOverlayItem(QGraphicsItem* parent)
    : QGraphicsItem(parent) {
    setZValue(-1.0);      // behind boundary and aircraft
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption); // gives correct exposedRect
}

void GridOverlayItem::setSpacingFt(int spacingFt) {
    if (m_spacingFt == spacingFt) return;
    m_spacingFt = spacingFt;
    update();
}

void GridOverlayItem::updateForView(double pixelsPerFt, QRectF visibleRect) {
    m_pixelsPerFt = pixelsPerFt;
    m_visibleRect = visibleRect;
    update();
}

QRectF GridOverlayItem::boundingRect() const {
    return QRectF(-kSceneBounds, -kSceneBounds,
                  2.0 * kSceneBounds, 2.0 * kSceneBounds);
}

void GridOverlayItem::paint(QPainter* painter,
                            const QStyleOptionGraphicsItem* /*option*/,
                            QWidget* /*widget*/) {
    if (!isVisible()) return;
    if (m_spacingFt <= 0) return;

    // Use the visible rect to limit drawing (performance).
    const QRectF& r = m_visibleRect.isNull() ? boundingRect() : m_visibleRect;
    if (r.isEmpty()) return;

    // Determine grid pen — thin, light gray, cosmetic (always 0.5 px on screen).
    QPen pen(QColor(0xCC, 0xCC, 0xCC));
    pen.setCosmetic(true);
    pen.setWidthF(0.5);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing, false);

    const double sp = static_cast<double>(m_spacingFt);

    // Snap left/top to nearest multiple of sp (rounding outward).
    const double left   = std::floor(r.left()   / sp) * sp;
    const double top    = std::floor(r.top()    / sp) * sp;
    const double right  = std::ceil (r.right()  / sp) * sp;
    const double bottom = std::ceil (r.bottom() / sp) * sp;

    // Clamp to scene bounds.
    const double xMin = std::max(left,   -kSceneBounds);
    const double xMax = std::min(right,   kSceneBounds);
    const double yMin = std::max(top,    -kSceneBounds);
    const double yMax = std::min(bottom,  kSceneBounds);

    // Vertical lines
    for (double x = xMin; x <= xMax + 0.01; x += sp) {
        painter->drawLine(QLineF(x, yMin, x, yMax));
    }

    // Horizontal lines
    for (double y = yMin; y <= yMax + 0.01; y += sp) {
        painter->drawLine(QLineF(xMin, y, xMax, y));
    }
}

} // namespace arld::ui
