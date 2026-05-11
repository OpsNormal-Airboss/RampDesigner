#include <arld/ui/RampView.h>
#include <arld/ui/RampScene.h>
#include <arld/core/Config.h>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace arld::ui {

RampView::RampView(QWidget* parent) : QGraphicsView(parent) {
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(NoDrag);
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorViewCenter);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFocusPolicy(Qt::StrongFocus);
    setBackgroundBrush(QColor(220, 225, 230));
    setAcceptDrops(true);
}

void RampView::setRampScene(RampScene* scene) {
    m_rampScene = scene;
    setScene(scene);
    applyScale();
    notifySceneOverlay();
}

double RampView::pixelsPerFt() const {
    // scale_factor = (12 inches/ft × screen_DPI) / scale_denominator
    return (12.0 * logicalDpiX()) / m_scaleDenominator;
}

void RampView::applyScale() {
    const double ppf = pixelsPerFt();
    resetTransform();
    scale(ppf, ppf);
}

void RampView::notifySceneOverlay() {
    if (!m_rampScene) return;
    // Position scale bar 12 px from the bottom-left of the viewport.
    QPointF scenePos = mapToScene(QPoint(12, height() - 12));
    m_rampScene->updateOverlay(pixelsPerFt(), scenePos);
}

void RampView::setScaleDenominator(double s) {
    s = std::clamp(s,
                   static_cast<double>(arld::core::kMinScaleDenominator),
                   static_cast<double>(arld::core::kMaxScaleDenominator));
    if (qFuzzyCompare(s, m_scaleDenominator)) return;
    m_scaleDenominator = s;
    applyScale();
    notifySceneOverlay();
    emit scaleChanged(m_scaleDenominator);
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void RampView::wheelEvent(QWheelEvent* event) {
    const int delta = event->angleDelta().y();
    if (delta == 0) { event->ignore(); return; }

    const double factor = (delta > 0) ? (1.0 / 1.15) : 1.15;
    setTransformationAnchor(AnchorUnderMouse);
    setScaleDenominator(m_scaleDenominator * factor);
    event->accept();
}

void RampView::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        setTransformationAnchor(AnchorViewCenter);
        setScaleDenominator(m_scaleDenominator / 1.25);
        break;
    case Qt::Key_Minus:
        setTransformationAnchor(AnchorViewCenter);
        setScaleDenominator(m_scaleDenominator * 1.25);
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

void RampView::mousePressEvent(QMouseEvent* event) {
    // Middle-click OR left-click on empty canvas → pan.
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && !itemAt(event->pos()))) {
        // Only pan in select mode; in draw mode, left-click draws.
        if (!m_rampScene || m_rampScene->editMode() == EditMode::Select ||
            event->button() == Qt::MiddleButton) {
            m_panning = true;
            m_lastPanPos = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void RampView::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        notifySceneOverlay();
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void RampView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_panning) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void RampView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    notifySceneOverlay();
}

void RampView::scrollContentsBy(int dx, int dy) {
    QGraphicsView::scrollContentsBy(dx, dy);
    notifySceneOverlay();
}

void RampView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-arld-aircraft-id"))
        event->acceptProposedAction();
    else
        event->ignore();
}

void RampView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat("application/x-arld-aircraft-id"))
        event->acceptProposedAction();
    else
        event->ignore();
}

void RampView::dropEvent(QDropEvent* event) {
    if (!event->mimeData()->hasFormat("application/x-arld-aircraft-id")) {
        event->ignore();
        return;
    }
    const QString id = QString::fromUtf8(
        event->mimeData()->data("application/x-arld-aircraft-id"));
    const QPointF scenePos = mapToScene(event->position().toPoint());
    emit aircraftDropped(id, scenePos);
    event->acceptProposedAction();
}

} // namespace arld::ui
