#pragma once
#include <QGraphicsItem>
#include <QRectF>

namespace arld::ui {

/// A grid overlay drawn behind all scene items.
/// Grid lines are drawn at configurable foot intervals.
/// Does NOT have Q_OBJECT — it is a plain QGraphicsItem.
class GridOverlayItem : public QGraphicsItem {
public:
    explicit GridOverlayItem(QGraphicsItem* parent = nullptr);

    /// Set grid spacing in feet (25, 50, or 100).
    void setSpacingFt(int spacingFt);
    int spacingFt() const { return m_spacingFt; }

    /// Update the visible region and pixels-per-ft for the current view state.
    void updateForView(double pixelsPerFt, QRectF visibleRect);

    QRectF boundingRect() const override;
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

private:
    int    m_spacingFt    = 50;
    QRectF m_visibleRect;
    double m_pixelsPerFt  = 1.0;
};

} // namespace arld::ui
