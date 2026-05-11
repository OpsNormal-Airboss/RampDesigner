#pragma once
#include <QGraphicsItem>

namespace arld::ui {

// Fixed-position overlay that renders a dynamic scale bar.
// Uses ItemIgnoresTransformations so it stays constant screen size.
// RampView repositions it whenever the viewport geometry changes.
class ScaleBarItem : public QGraphicsItem {
public:
    explicit ScaleBarItem(QGraphicsItem* parent = nullptr);

    void setPixelsPerFt(double ppf);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
    void recalculate();

    double m_pixelsPerFt = 1.0;
    double m_barFt = 100.0;      // represented real-world distance
    double m_barPixels = 100.0;  // corresponding screen width
};

} // namespace arld::ui
