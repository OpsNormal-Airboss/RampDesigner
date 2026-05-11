#include <arld/ui/RampScene.h>
#include <arld/ui/RampBoundaryItem.h>
#include <arld/ui/ScaleBarItem.h>
#include <arld/core/Config.h>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <cmath>

namespace arld::ui {

// ---------------------------------------------------------------------------
// Command defined here — operates on RampBoundaryItem (same translation unit)
// ---------------------------------------------------------------------------
namespace {

class BoundaryAddPointCommand : public arld::core::ICommand {
public:
    BoundaryAddPointCommand(RampBoundaryItem* item, QPointF p)
        : m_item(item), m_point(p) {}
    void execute() override { m_item->addPoint(m_point); }
    void undo() override    { m_item->removeLastPoint(); }
    std::string describe() const override { return "Add Boundary Point"; }
private:
    RampBoundaryItem* m_item;
    QPointF m_point;
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// RampScene
// ---------------------------------------------------------------------------
RampScene::RampScene(QObject* parent)
    : QGraphicsScene(parent)
    , m_undoStack(arld::core::kUndoHistoryDepth) {
    // Default scene rect: 20 000 ft × 20 000 ft centred on origin.
    setSceneRect(-10000, -10000, 20000, 20000);

    m_boundaryItem = new RampBoundaryItem();
    addItem(m_boundaryItem);

    m_boundaryItem->onCommandReady = [this](std::unique_ptr<arld::core::ICommand> cmd) {
        // Vertex-drag commands arrive already applied visually; push without
        // re-executing by inserting directly (bypass push's execute call).
        // We use a thin wrapper that skips the first execute().
        struct AlreadyExecutedWrapper : arld::core::ICommand {
            std::unique_ptr<arld::core::ICommand> inner;
            bool firstExecuteDone = false;
            explicit AlreadyExecutedWrapper(std::unique_ptr<arld::core::ICommand> c)
                : inner(std::move(c)) {}
            void execute() override {
                if (firstExecuteDone) inner->execute();
                firstExecuteDone = true;
            }
            void undo() override { inner->undo(); }
            std::string describe() const override { return inner->describe(); }
        };
        m_undoStack.push(std::make_unique<AlreadyExecutedWrapper>(std::move(cmd)));
    };

    m_scaleBarItem = new ScaleBarItem();
    addItem(m_scaleBarItem);
}

void RampScene::setEditMode(EditMode mode) {
    m_mode = mode;
    emit editModeChanged(mode);
}

void RampScene::updateOverlay(double pixelsPerFt, QPointF scaleBarScenePos) {
    m_scaleBarItem->setPixelsPerFt(pixelsPerFt);
    m_scaleBarItem->setPos(scaleBarScenePos);
}

QPointF RampScene::snapToGrid(QPointF pos, bool freehand) const {
    if (freehand) return pos;
    const float g = arld::core::kDefaultGridSpacingFt;
    return QPointF(std::round(pos.x() / g) * g,
                   std::round(pos.y() / g) * g);
}

void RampScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (m_mode == EditMode::DrawBoundary && event->button() == Qt::LeftButton) {
        bool freehand = event->modifiers() & Qt::ShiftModifier;
        QPointF pos = snapToGrid(event->scenePos(), freehand);
        m_undoStack.push(std::make_unique<BoundaryAddPointCommand>(m_boundaryItem, pos));
        event->accept();
        return;
    }
    QGraphicsScene::mousePressEvent(event);
}

void RampScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    if (m_mode == EditMode::DrawBoundary && event->button() == Qt::LeftButton) {
        // mousePressEvent already added the second-click point — undo it.
        if (m_undoStack.canUndo()) m_undoStack.undo();

        if (m_boundaryItem->pointCount() >= 3) {
            m_boundaryItem->closePolygon();
            setEditMode(EditMode::Select);
        }
        event->accept();
        return;
    }
    QGraphicsScene::mouseDoubleClickEvent(event);
}

void RampScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsScene::mouseMoveEvent(event);
}

} // namespace arld::ui
