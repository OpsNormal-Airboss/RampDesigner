#include <arld/ui/RampView.h>
#include <arld/ui/RampScene.h>
#include <arld/ui/AircraftItem.h>
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
    setRubberBandSelectionMode(Qt::IntersectsItemShape);
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
    // scene() is nulled by QGraphicsView::setScene(nullptr) before scrollbar
    // recalculation fires; m_rampScene is never explicitly cleared, so checking
    // only m_rampScene misses the window where the scene is mid-destruction.
    if (!m_rampScene || !scene()) return;
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

    // LOD: switch to simplified rect rendering when zoomed out past 1:2000.
    if (scene()) {
        const bool simplified = (m_scaleDenominator > 2000.0);
        for (auto* item : scene()->items()) {
            if (auto* ai = qgraphicsitem_cast<arld::ui::AircraftItem*>(item))
                ai->setLodSimplified(simplified);
        }
    }
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
    case Qt::Key_Escape:
        if (m_rampScene) m_rampScene->clearSelection();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

void RampView::mousePressEvent(QMouseEvent* event) {
    // Middle-click → always pan.
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // Only items with selection/move capability count as "on item" — this
        // prevents GridOverlayItem and other passive scene decorations from
        // blocking rubber-band lasso when the user clicks on empty canvas.
        const auto itemsHere = items(event->pos());
        const bool onItem = std::any_of(itemsHere.cbegin(), itemsHere.cend(),
            [](const QGraphicsItem* i) {
                const auto f = i->flags();
                return (f & QGraphicsItem::ItemIsSelectable)
                    || (f & QGraphicsItem::ItemIsMovable);
            });
        const bool inSelectMode = !m_rampScene || m_rampScene->editMode() == EditMode::Select;

        if (inSelectMode) {
            if (!onItem) {
                // Left-click on empty canvas: start rubber-band lasso.
                const bool shiftHeld = event->modifiers() & Qt::ShiftModifier;
                if (!shiftHeld && m_rampScene)
                    m_rampScene->clearSelection();
                // With Shift, save current selection so we can merge it back
                // after the rubber band replaces it.
                m_shiftSelectionSave = (shiftHeld && m_rampScene)
                    ? m_rampScene->selectedItems()
                    : QList<QGraphicsItem*>{};
                setDragMode(RubberBandDrag);
                QGraphicsView::mousePressEvent(event);
                return;
            } else {
                // Left-click on an item: pass to scene for item interaction.
                setDragMode(NoDrag);
                m_shiftSelectionSave.clear();
            }
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
    const bool wasRubberBand = (dragMode() == RubberBandDrag);
    QGraphicsView::mouseReleaseEvent(event);
    if (wasRubberBand) {
        setDragMode(NoDrag);
        // Merge items saved before Shift+lasso back into selection.
        for (auto* item : std::as_const(m_shiftSelectionSave))
            item->setSelected(true);
        m_shiftSelectionSave.clear();
    }
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
