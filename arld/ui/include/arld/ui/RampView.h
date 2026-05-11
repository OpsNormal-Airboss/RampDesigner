#pragma once
#include <QGraphicsView>
#include <QList>

namespace arld::ui {

class RampScene;

class RampView : public QGraphicsView {
    Q_OBJECT

public:
    explicit RampView(QWidget* parent = nullptr);

    void setRampScene(RampScene* scene);

    double scaleDenominator() const { return m_scaleDenominator; }
    void setScaleDenominator(double s);

signals:
    void scaleChanged(double denominator);
    void aircraftDropped(const QString& aircraftId, QPointF scenePos);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void applyScale();
    double pixelsPerFt() const;
    void notifySceneOverlay();

    RampScene* m_rampScene = nullptr;
    double m_scaleDenominator = 1200.0;
    bool m_panning = false;
    QPoint m_lastPanPos;
    QList<QGraphicsItem*> m_shiftSelectionSave; // items to preserve across Shift+lasso
};

} // namespace arld::ui
