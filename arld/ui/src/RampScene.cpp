#include <arld/ui/RampScene.h>
#include <arld/ui/AircraftItem.h>
#include <arld/ui/ClearanceZoneItem.h>
#include <arld/ui/RampBoundaryItem.h>
#include <arld/ui/ScaleBarItem.h>
#include <arld/core/Config.h>
#include <arld/core/ClearanceEngine.h>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <cmath>
#include <unordered_map>

namespace arld::ui {

// ---------------------------------------------------------------------------
// Commands
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
    setSceneRect(-10000, -10000, 20000, 20000);

    m_boundaryItem = new RampBoundaryItem();
    addItem(m_boundaryItem);

    m_boundaryItem->onCommandReady = [this](std::unique_ptr<arld::core::ICommand> cmd) {
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

    // Debounce clearance recomputation: wait 80 ms after the last scene change
    // before running the O(N²) check so dragging doesn't block the UI.
    m_clearanceTimer.setSingleShot(true);
    m_clearanceTimer.setInterval(80);
    connect(&m_clearanceTimer, &QTimer::timeout,
            this, &RampScene::recomputeClearance);

    connect(this, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) {
        m_clearanceTimer.start();  // restart debounce window
    });
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

void RampScene::placeAircraft(const arld::core::AircraftLibraryEntry& entry, QPointF scenePos) {
    const QString svgPath =
        QString(":/library/silhouettes/%1").arg(QString::fromStdString(entry.silhouetteSvg));

    auto* aircraft = new AircraftItem(entry, svgPath);

    aircraft->setPos(scenePos);
    const QRectF br = aircraft->childrenBoundingRect();
    aircraft->setPos(scenePos - br.center());

    aircraft->onCommandReady = [this](std::unique_ptr<arld::core::ICommand> cmd) {
        struct AlreadyExecuted : arld::core::ICommand {
            std::unique_ptr<arld::core::ICommand> inner;
            bool done = false;
            explicit AlreadyExecuted(std::unique_ptr<arld::core::ICommand> c)
                : inner(std::move(c)) {}
            void execute() override { if (done) inner->execute(); done = true; }
            void undo()    override { inner->undo(); }
            std::string describe() const override { return inner->describe(); }
        };
        m_undoStack.push(std::make_unique<AlreadyExecuted>(std::move(cmd)));
    };

    struct PlaceCmd : arld::core::ICommand {
        AircraftItem* item;
        explicit PlaceCmd(AircraftItem* a) : item(a) {}
        void execute() override { item->setVisible(true); }
        void undo()    override { item->setVisible(false); }
        std::string describe() const override { return "Place Aircraft"; }
    };

    struct AlreadyExecuted : arld::core::ICommand {
        std::unique_ptr<arld::core::ICommand> inner;
        bool done = false;
        explicit AlreadyExecuted(std::unique_ptr<arld::core::ICommand> c)
            : inner(std::move(c)) {}
        void execute() override { if (done) inner->execute(); done = true; }
        void undo()    override { inner->undo(); }
        std::string describe() const override { return inner->describe(); }
    };

    addItem(aircraft);
    m_aircraft.push_back(aircraft);

    m_undoStack.push(std::make_unique<AlreadyExecuted>(
        std::make_unique<PlaceCmd>(aircraft)));
}

void RampScene::recomputeClearance() {
    // Collect states for all visible (placed and not undone) aircraft.
    std::vector<arld::core::AircraftState> states;
    states.reserve(m_aircraft.size());
    std::vector<AircraftItem*> visible;
    visible.reserve(m_aircraft.size());

    for (auto* item : m_aircraft) {
        if (item->isVisible()) {
            states.push_back(item->toAircraftState());
            visible.push_back(item);
        }
    }

    if (states.empty()) {
        emit violationCountChanged(0);
        return;
    }

    const auto violations = arld::core::ClearanceEngine::detectViolations(states);

    // Reset all clearance zones to Clear.
    for (auto* item : visible) {
        if (auto* zone = item->clearanceItem())
            zone->setStatus(arld::core::ClearanceSeverity::Clear);
    }

    // Build a quick id → AircraftItem* lookup.
    std::unordered_map<std::string, AircraftItem*> lookup;
    for (auto* item : visible)
        lookup[item->entry().id] = item;

    // Apply the worst severity per aircraft (a single aircraft can have multiple violations).
    int violationPairs = 0;
    for (const auto& v : violations) {
        if (v.severity == arld::core::ClearanceSeverity::Violation) ++violationPairs;

        auto applyWorst = [](ClearanceZoneItem* zone, arld::core::ClearanceSeverity sev) {
            if (!zone) return;
            // Only upgrade, never downgrade a zone already in a worse state.
            using S = arld::core::ClearanceSeverity;
            if (sev == S::Violation ||
                (sev == S::Advisory && zone->status() == S::Clear)) {
                zone->setStatus(sev);
            }
        };

        if (auto it = lookup.find(v.idA); it != lookup.end())
            applyWorst(it->second->clearanceItem(), v.severity);
        if (auto it = lookup.find(v.idB); it != lookup.end())
            applyWorst(it->second->clearanceItem(), v.severity);
    }

    emit violationCountChanged(violationPairs);
}

} // namespace arld::ui
