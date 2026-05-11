#include <arld/ui/RampBoundaryItem.h>
#include <arld/core/Config.h>
#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <cmath>

namespace arld::ui {

// ---------------------------------------------------------------------------
// VertexHandle — draggable handle representing one polygon vertex
// ---------------------------------------------------------------------------
class VertexHandle : public QGraphicsEllipseItem {
public:
    VertexHandle(int index, RampBoundaryItem* boundary)
        : QGraphicsEllipseItem(-5, -5, 10, 10, boundary)
        , m_index(index)
        , m_boundary(boundary) {
        setFlag(ItemIsMovable);
        setFlag(ItemSendsGeometryChanges);
        setFlag(ItemIgnoresTransformations); // handle stays 10px regardless of zoom
        setPen(QPen(QColor(60, 120, 230), 1.5));
        setBrush(Qt::white);
        setCursor(Qt::CrossCursor);
        setZValue(10);
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
        if (change == ItemPositionChange && scene()) {
            QPointF newPos = value.toPointF();
            bool freehand = QApplication::keyboardModifiers() & Qt::ShiftModifier;
            if (!freehand) {
                const float g = arld::core::kDefaultGridSpacingFt;
                newPos = QPointF(std::round(newPos.x() / g) * g,
                                 std::round(newPos.y() / g) * g);
            }
            m_boundary->onVertexMoved(m_index, newPos);
            return newPos;
        }
        return QGraphicsEllipseItem::itemChange(change, value);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        // Record pre-drag position so we can create an undo command on release.
        m_dragStart = pos();
        QGraphicsEllipseItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        QGraphicsEllipseItem::mouseReleaseEvent(event);
        m_boundary->onVertexDragFinished(m_index, m_dragStart);
    }

private:
    int m_index;
    RampBoundaryItem* m_boundary;
    QPointF m_dragStart;
};

// ---------------------------------------------------------------------------
// Commands — defined here so they can access RampBoundaryItem methods
// ---------------------------------------------------------------------------
namespace {

class MoveVertexCommand : public arld::core::ICommand {
public:
    MoveVertexCommand(RampBoundaryItem* item, int index, QPointF before, QPointF after)
        : m_item(item), m_index(index), m_before(before), m_after(after) {}
    void execute() override { m_item->moveVertex(m_index, m_after); }
    void undo() override    { m_item->moveVertex(m_index, m_before); }
    std::string describe() const override { return "Move Vertex"; }
private:
    RampBoundaryItem* m_item;
    int m_index;
    QPointF m_before, m_after;
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// RampBoundaryItem
// ---------------------------------------------------------------------------
RampBoundaryItem::RampBoundaryItem(QGraphicsItem* parent)
    : QGraphicsPathItem(parent) {
    QPen pen(QColor(30, 100, 220));
    pen.setWidthF(2.0);
    pen.setCosmetic(true); // constant 2-px width regardless of zoom
    setPen(pen);
    setBrush(QColor(100, 160, 255, 40));
    setZValue(1);
}

void RampBoundaryItem::addPoint(QPointF p) {
    m_points.append(p);
    rebuildPath();
    syncHandles();
}

void RampBoundaryItem::removeLastPoint() {
    if (m_points.isEmpty()) return;
    m_points.removeLast();
    rebuildPath();
    syncHandles();
}

void RampBoundaryItem::moveVertex(int index, QPointF pos) {
    if (index < 0 || index >= m_points.size()) return;
    m_points[index] = pos;
    // Sync the handle position without triggering another itemChange cascade.
    if (index < m_handles.size()) {
        m_handles[index]->setFlag(ItemSendsGeometryChanges, false);
        m_handles[index]->setPos(pos);
        m_handles[index]->setFlag(ItemSendsGeometryChanges, true);
    }
    rebuildPath();
}

void RampBoundaryItem::closePolygon() {
    if (m_points.size() < 3 || m_closed) return;
    m_closed = true;
    rebuildPath();
}

void RampBoundaryItem::clearBoundary() {
    qDeleteAll(m_handles);
    m_handles.clear();
    m_points.clear();
    m_closed = false;
    rebuildPath();
}

void RampBoundaryItem::onVertexMoved(int index, QPointF scenePos) {
    if (index < 0 || index >= m_points.size()) return;
    m_points[index] = scenePos;
    rebuildPath();
}

void RampBoundaryItem::onVertexDragFinished(int index, QPointF dragStart) {
    if (!onCommandReady) return;
    QPointF after = m_points.value(index);
    if (after == dragStart) return; // no net movement
    onCommandReady(std::make_unique<MoveVertexCommand>(this, index, dragStart, after));
}

void RampBoundaryItem::rebuildPath() {
    if (m_points.isEmpty()) { setPath({}); return; }

    QPainterPath path;
    path.moveTo(m_points.first());
    for (int i = 1; i < m_points.size(); ++i)
        path.lineTo(m_points[i]);
    if (m_closed)
        path.closeSubpath();

    setPath(path);
}

void RampBoundaryItem::syncHandles() {
    // Add missing handles.
    while (m_handles.size() < m_points.size()) {
        int idx = m_handles.size();
        auto* h = new VertexHandle(idx, this);
        h->setPos(m_points[idx]);
        m_handles.append(h);
    }
    // Remove excess handles (after removeLastPoint).
    while (m_handles.size() > m_points.size()) {
        delete m_handles.takeLast();
    }
    // Sync positions.
    for (int i = 0; i < m_handles.size(); ++i)
        m_handles[i]->setPos(m_points[i]);
}

} // namespace arld::ui
