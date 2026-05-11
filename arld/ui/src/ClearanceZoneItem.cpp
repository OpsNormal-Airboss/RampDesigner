#include <arld/ui/ClearanceZoneItem.h>
#include <arld/core/Config.h>
#include <QPen>
#include <QBrush>

namespace arld::ui {

ClearanceZoneItem::ClearanceZoneItem(QGraphicsItem* parent)
    : QGraphicsPolygonItem(parent) {
    setZValue(-0.5);  // render behind the SVG silhouette
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    updateAppearance();
}

void ClearanceZoneItem::setStatus(arld::core::ClearanceSeverity status) {
    if (m_status == status) return;
    m_status = status;
    updateAppearance();
}

void ClearanceZoneItem::updateAppearance() {
    using S = arld::core::ClearanceSeverity;

    QColor fill;
    QColor outline;

    switch (m_status) {
        case S::Clear:
            fill    = QColor(0x22, 0xAA, 0x44,
                             static_cast<int>(arld::core::kAdvisoryZoneOpacity * 255));
            outline = QColor(0x00, 0x88, 0x22, 180);
            break;
        case S::Advisory:
            fill    = QColor(0xDD, 0xAA, 0x00,
                             static_cast<int>(arld::core::kAdvisoryZoneOpacity * 255));
            outline = QColor(0xBB, 0x88, 0x00, 200);
            break;
        case S::Violation:
            fill    = QColor(0xCC, 0x22, 0x22,
                             static_cast<int>(arld::core::kViolationZoneOpacity * 255));
            outline = QColor(0xAA, 0x00, 0x00, 220);
            break;
        case S::Overridden:
            fill    = QColor(0xDD, 0x77, 0x00,
                             static_cast<int>(0.35f * 255));
            outline = QColor(0xBB, 0x55, 0x00, 210);
            break;
    }

    setBrush(QBrush(fill));
    setPen(QPen(outline, 0.5, Qt::DashLine));
}

} // namespace arld::ui
