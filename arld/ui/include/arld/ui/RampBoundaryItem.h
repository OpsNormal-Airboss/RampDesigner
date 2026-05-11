#pragma once
#include <arld/core/ICommand.h>
#include <QGraphicsPathItem>
#include <QVector>
#include <functional>
#include <memory>

namespace arld::ui {

class VertexHandle;

class RampBoundaryItem : public QGraphicsPathItem {
public:
    explicit RampBoundaryItem(QGraphicsItem* parent = nullptr);

    void addPoint(QPointF p);
    void removeLastPoint();
    void moveVertex(int index, QPointF pos);
    void closePolygon();

    bool isClosed() const { return m_closed; }
    int pointCount() const { return m_points.size(); }
    QPointF point(int i) const { return m_points.at(i); }

    // RampScene sets this to route vertex-drag commands into the undo stack.
    std::function<void(std::unique_ptr<arld::core::ICommand>)> onCommandReady;

    // Called by VertexHandle during drag (live visual update, no undo entry).
    void onVertexMoved(int index, QPointF scenePos);
    // Called by VertexHandle on mouse-release to push the undo command.
    void onVertexDragFinished(int index, QPointF dragStart);

private:
    void rebuildPath();
    void syncHandles();

    QVector<QPointF> m_points;
    QVector<VertexHandle*> m_handles;
    QVector<QPointF> m_dragStartPos; // per-handle start position for undo
    bool m_closed = false;
};

} // namespace arld::ui
