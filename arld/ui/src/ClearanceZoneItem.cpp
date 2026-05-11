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

void ClearanceZoneItem::setDisplayType(arld::core::DisplayType dt) {
    if (m_displayType == dt) return;
    m_displayType = dt;
    updateAppearance();
}

void ClearanceZoneItem::updateAppearance() {
    using S  = arld::core::ClearanceSeverity;
    using DT = arld::core::DisplayType;

    QColor fill;
    QColor outline;
    Qt::BrushStyle brushStyle = Qt::SolidPattern;
    qreal  penWidth  = 0.5;
    Qt::PenStyle penStyle = Qt::DashLine;

    // --- Status-driven color and CVD pattern ---
    switch (m_status) {
        case S::Clear:
            fill    = QColor(0x22, 0xAA, 0x44,
                             static_cast<int>(arld::core::kAdvisoryZoneOpacity * 255));
            outline = QColor(0x00, 0x88, 0x22, 180);
            brushStyle = Qt::SolidPattern;   // clear: solid green
            break;
        case S::Advisory:
            fill    = QColor(0xDD, 0xAA, 0x00,
                             static_cast<int>(arld::core::kAdvisoryZoneOpacity * 255));
            outline = QColor(0xBB, 0x88, 0x00, 200);
            brushStyle = Qt::BDiagPattern;   // advisory: diagonal lines (CVD)
            break;
        case S::Violation:
            fill    = QColor(0xCC, 0x22, 0x22,
                             static_cast<int>(arld::core::kViolationZoneOpacity * 255));
            outline = QColor(0xAA, 0x00, 0x00, 220);
            brushStyle = Qt::DiagCrossPattern; // violation: cross-hatch (CVD)
            break;
        case S::Overridden:
            fill    = QColor(0xDD, 0x77, 0x00,
                             static_cast<int>(0.35f * 255));
            outline = QColor(0xBB, 0x55, 0x00, 210);
            brushStyle = Qt::HorPattern;     // overridden: horizontal lines (CVD)
            break;
    }

    // --- Display-type-aware pen adjustments ---
    switch (m_displayType) {
        case DT::MilitaryStatic:
            penWidth = 1.5;
            penStyle = Qt::DashLine;
            outline  = QColor(0xAA, 0x00, 0x00, 230); // thick red dashed border
            break;
        case DT::RampShow:
            penWidth = 1.5;
            penStyle = Qt::DashDotLine; // thick amber dashed outer boundary
            break;
        case DT::TaxiOnly:
            penWidth = 0.75;
            penStyle = Qt::DotLine;
            break;
        default:
            break;
    }

    setBrush(QBrush(fill, brushStyle));
    setPen(QPen(outline, penWidth, penStyle));
}

} // namespace arld::ui
