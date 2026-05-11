#include <arld/ui/ScaleBarItem.h>
#include <QPainter>
#include <QFont>
#include <cmath>
#include <array>

namespace arld::ui {

// Candidate round-number distances for the scale bar label, in feet.
static constexpr std::array<double, 10> kCandidates{
    10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000
};

ScaleBarItem::ScaleBarItem(QGraphicsItem* parent) : QGraphicsItem(parent) {
    setFlag(ItemIgnoresTransformations);
    setZValue(100);
}

void ScaleBarItem::setPixelsPerFt(double ppf) {
    m_pixelsPerFt = ppf;
    recalculate();
    update();
}

void ScaleBarItem::recalculate() {
    // Pick the candidate that gives a bar between 80 and 200 screen pixels wide.
    constexpr double kTargetPixels = 120.0;
    double targetFt = kTargetPixels / m_pixelsPerFt;

    m_barFt = kCandidates.back();
    for (double c : kCandidates) {
        if (c >= targetFt) { m_barFt = c; break; }
    }
    m_barPixels = m_barFt * m_pixelsPerFt;
}

QRectF ScaleBarItem::boundingRect() const {
    return QRectF(-4, -20, m_barPixels + 8, 28);
}

void ScaleBarItem::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem*,
                         QWidget*) {
    painter->save();

    // Background pill for readability.
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, 180));
    painter->drawRoundedRect(boundingRect(), 3, 3);

    // Scale bar line and end ticks.
    QPen barPen(QColor(40, 40, 40), 2.0);
    painter->setPen(barPen);
    painter->drawLine(QPointF(0, 0), QPointF(m_barPixels, 0));
    painter->drawLine(QPointF(0, -5), QPointF(0, 0));
    painter->drawLine(QPointF(m_barPixels, -5), QPointF(m_barPixels, 0));

    // Label — e.g. "100 ft" or "1000 ft".
    QString label;
    if (m_barFt >= 1000)
        label = QString::number(static_cast<int>(m_barFt / 1000)) + QStringLiteral(",000 ft");
    else
        label = QString::number(static_cast<int>(m_barFt)) + QStringLiteral(" ft");

    QFont font;
    font.setPointSizeF(8.0);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(QColor(30, 30, 30));
    painter->drawText(QRectF(0, -18, m_barPixels, 14),
                      Qt::AlignHCenter | Qt::AlignVCenter, label);

    painter->restore();
}

} // namespace arld::ui
