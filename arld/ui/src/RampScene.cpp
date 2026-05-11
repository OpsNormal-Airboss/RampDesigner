#include <arld/ui/RampScene.h>
#include <arld/ui/AircraftItem.h>
#include <arld/ui/ClearanceZoneItem.h>
#include <arld/ui/GridOverlayItem.h>
#include <arld/ui/RampBoundaryItem.h>
#include <arld/ui/ScaleBarItem.h>
#include <arld/core/Config.h>
#include <arld/core/ClearanceEngine.h>
#include <arld/core/ClearanceRuleSet.h>
#include <arld/core/ProjectFile.h>
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
    , m_undoStack(arld::core::kUndoHistoryDepth)
    , m_ruleSet(arld::core::ClearanceRuleSet::faaCoW()) {
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

    m_gridOverlay = new GridOverlayItem();
    addItem(m_gridOverlay);

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

    if (m_gridOverlay) {
        // Pass the full scene rect as the visible region hint;
        // RampView could pass the viewport rect, but sceneRect() is acceptable.
        m_gridOverlay->updateForView(pixelsPerFt, sceneRect());
    }
}

void RampScene::setGridVisible(bool visible) {
    if (m_gridOverlay) m_gridOverlay->setVisible(visible);
}

void RampScene::setGridSpacingFt(int spacingFt) {
    if (m_gridOverlay) m_gridOverlay->setSpacingFt(spacingFt);
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

void RampScene::setRuleSet(const arld::core::ClearanceRuleSet& rs) {
    m_ruleSet = rs;
    recomputeClearance();
}

void RampScene::addOverride(const arld::core::ClearanceOverride& ov) {
    m_overrides.push_back(ov);
    recomputeClearance();
}

QPointF RampScene::aircraftSceneCenter(const std::string& id) const {
    for (auto* item : m_aircraft) {
        if (item->placementId() == id && item->isVisible()) {
            const auto s = item->toAircraftState();
            return QPointF(s.centerX, s.centerY);
        }
    }
    return QPointF{};
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
        m_lastViolations.clear();
        emit violationCountChanged(0);
        emit violationsChanged();
        return;
    }

    auto violations = arld::core::ClearanceEngine::detectViolations(states, m_ruleSet);

    // Apply overrides: change severity from Violation to Overridden for matching pairs.
    for (auto& v : violations) {
        if (v.severity != arld::core::ClearanceSeverity::Violation) continue;
        for (const auto& ov : m_overrides) {
            if ((ov.placementIdA == v.idA && ov.placementIdB == v.idB) ||
                (ov.placementIdA == v.idB && ov.placementIdB == v.idA)) {
                v.severity = arld::core::ClearanceSeverity::Overridden;
                break;
            }
        }
    }

    m_lastViolations = violations;

    // Reset all clearance zones to Clear.
    for (auto* item : visible) {
        if (auto* zone = item->clearanceItem())
            zone->setStatus(arld::core::ClearanceSeverity::Clear);
    }

    // Build a quick id → AircraftItem* lookup (by placement id).
    std::unordered_map<std::string, AircraftItem*> lookup;
    for (auto* item : visible)
        lookup[item->placementId()] = item;

    // Apply the worst severity per aircraft (a single aircraft can have multiple violations).
    int violationPairs = 0;
    for (const auto& v : violations) {
        if (v.severity == arld::core::ClearanceSeverity::Violation) ++violationPairs;

        auto applyWorst = [](ClearanceZoneItem* zone, arld::core::ClearanceSeverity sev) {
            if (!zone) return;
            using S = arld::core::ClearanceSeverity;
            // Priority: Violation > Overridden > Advisory > Clear
            const auto cur = zone->status();
            if (sev == S::Violation) {
                zone->setStatus(sev);
            } else if (sev == S::Overridden && cur == S::Clear) {
                zone->setStatus(sev);
            } else if (sev == S::Advisory && cur == S::Clear) {
                zone->setStatus(sev);
            }
        };

        if (auto it = lookup.find(v.idA); it != lookup.end())
            applyWorst(it->second->clearanceItem(), v.severity);
        if (auto it = lookup.find(v.idB); it != lookup.end())
            applyWorst(it->second->clearanceItem(), v.severity);
    }

    emit violationCountChanged(violationPairs);
    emit violationsChanged();
}

// ---------------------------------------------------------------------------
// Sprint 0-5: clearScene / toProjectData / loadProjectData
// ---------------------------------------------------------------------------

void RampScene::clearScene() {
    // Remove all aircraft items from the scene.
    for (auto* item : m_aircraft) {
        removeItem(item);
        delete item;
    }
    m_aircraft.clear();
    m_overrides.clear();
    m_lastViolations.clear();

    // Reset boundary.
    m_boundaryItem->clearBoundary();

    // Clear undo history.
    m_undoStack.clear();
}

arld::core::ProjectData RampScene::toProjectData() const {
    arld::core::ProjectData data;
    data.arldVersion   = "1.1.0";
    data.schemaVersion = 2;
    data.metadata.modifiedUtc = arld::core::ProjectFile::currentUtcTimestamp();

    // Boundary
    for (int i = 0; i < m_boundaryItem->pointCount(); ++i) {
        const QPointF p = m_boundaryItem->point(i);
        data.boundary.vertices.emplace_back(
            static_cast<float>(p.x()),
            static_cast<float>(p.y()));
    }
    data.boundary.closed = m_boundaryItem->isClosed();

    // Aircraft (visible only — hidden items have been undone)
    for (const auto* item : m_aircraft) {
        if (!item->isVisible()) continue;
        const auto state = item->toAircraftState();
        arld::core::PlacedAircraft pa;
        pa.placementId  = item->placementId();
        pa.libraryId    = item->entry().id;
        pa.displayName  = item->entry().displayName;
        pa.centerX      = state.centerX;
        pa.centerY      = state.centerY;
        pa.rotationDeg  = state.rotationDeg;
        pa.wingspanFt   = item->entry().wingspanFt;
        pa.lengthFt     = item->entry().lengthFt;
        pa.displayType  = item->displayType();
        pa.tailNumber   = item->tailNumber();
        pa.owner        = item->owner();
        pa.fuelType     = item->fuelType();
        pa.hasHazmat    = item->hasHazmat();
        data.aircraft.push_back(std::move(pa));
    }

    // Overrides
    data.overrides = m_overrides;

    return data;
}

void RampScene::loadProjectData(
    const arld::core::ProjectData& data,
    std::function<const arld::core::AircraftLibraryEntry*(const std::string&)> lookup)
{
    clearScene();

    // Restore metadata title so callers can read it back if needed.
    // (No separate title storage on RampScene; MainWindow reads from data.)

    // Restore boundary.
    for (const auto& [x, y] : data.boundary.vertices)
        m_boundaryItem->addPoint(QPointF(static_cast<double>(x), static_cast<double>(y)));
    if (data.boundary.closed && m_boundaryItem->pointCount() >= 3)
        m_boundaryItem->closePolygon();

    // Restore overrides.
    m_overrides = data.overrides;

    // Restore aircraft (no undo push — this is a load, not a user action).
    for (const auto& pa : data.aircraft) {
        const auto* entry = lookup(pa.libraryId);
        if (!entry) continue;   // library entry not found; skip gracefully

        const QString svgPath =
            QString(":/library/silhouettes/%1")
                .arg(QString::fromStdString(entry->silhouetteSvg));

        auto* aircraft = new AircraftItem(*entry, svgPath);

        // Position: center_x/center_y are the scene-space center of the aircraft.
        const QRectF br = aircraft->childrenBoundingRect();
        const QPointF localCenter = br.center();
        aircraft->setPos(
            QPointF(static_cast<double>(pa.centerX), static_cast<double>(pa.centerY))
            - localCenter);
        aircraft->setRotation(static_cast<double>(pa.rotationDeg));
        aircraft->setDisplayType(pa.displayType);
        aircraft->setPlacementId(pa.placementId);
        aircraft->setTailNumber(pa.tailNumber);
        aircraft->setOwner(pa.owner);
        aircraft->setFuelType(pa.fuelType);
        aircraft->setHazmat(pa.hasHazmat);

        // Wire up command routing (same as placeAircraft).
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

        addItem(aircraft);
        m_aircraft.push_back(aircraft);
    }

    recomputeClearance();
}

} // namespace arld::ui
