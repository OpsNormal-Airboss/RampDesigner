#include <arld/ui/AircraftItem.h>
#include <arld/ui/ClearanceZoneItem.h>
#include <arld/core/ICommand.h>
#include <arld/core/ClearanceEngine.h>
#include <arld/core/Config.h>
#include <arld/core/GeomTypes.h>
#include <arld/core/ProjectFile.h>
#include <QGuiApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QSvgRenderer>
#include <QAction>
#include <QColor>
#include <QCursor>
#include <QFont>
#include <QMenu>
#include <QPolygonF>
#include <QPen>
#include <QBrush>
#include <cmath>

namespace arld::ui {

// ---------------------------------------------------------------------------
// Rotation handle — a small fixed-size circle the user drags to rotate.
// ---------------------------------------------------------------------------
class RotationHandle : public QGraphicsEllipseItem {
public:
    explicit RotationHandle(AircraftItem* parent)
        : QGraphicsEllipseItem(-5, -5, 10, 10, parent)
        , m_aircraft(parent) {
        setFlags(ItemIsMovable | ItemSendsGeometryChanges | ItemIgnoresTransformations);
        setCursor(Qt::CrossCursor);
        setBrush(QColor(0x00, 0xCC, 0x00, 200));
        setPen(QPen(QColor(0x00, 0x88, 0x00), 1.0));
        setZValue(1.0);
        setPos(m_aircraft->boundingRect().center().x(), -15.0);
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
        if (change == ItemPositionChange && scene()) {
            const QPointF newPos = value.toPointF();
            const QPointF center = m_aircraft->mapToScene(
                m_aircraft->boundingRect().center());
            const QPointF handleScene = m_aircraft->mapToScene(newPos);
            const double dx = handleScene.x() - center.x();
            const double dy = handleScene.y() - center.y();
            double angle = std::atan2(dx, -dy) * 180.0 / M_PI;

            const bool freehand = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
            if (!freehand)
                angle = std::round(angle / 45.0) * 45.0;
            else
                angle = std::round(angle);  // 1-degree precision

            m_aircraft->setRotation(angle);

            const double radius = 15.0;
            const double rad = angle * M_PI / 180.0;
            return QPointF(m_aircraft->boundingRect().center().x() + radius * std::sin(rad),
                           m_aircraft->boundingRect().center().y() - radius * std::cos(rad));
        }
        return QGraphicsEllipseItem::itemChange(change, value);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        m_startAngle = m_aircraft->rotation();
        QGraphicsEllipseItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        QGraphicsEllipseItem::mouseReleaseEvent(event);
        const double endAngle = m_aircraft->rotation();
        if (std::abs(endAngle - m_startAngle) > 0.01)
            m_aircraft->finishRotation(m_startAngle, endAngle);
    }

private:
    AircraftItem* m_aircraft;
    double m_startAngle = 0.0;
};

// ---------------------------------------------------------------------------
// Commands (anonymous namespace)
// ---------------------------------------------------------------------------
namespace {

class MoveAircraftCommand : public arld::core::ICommand {
public:
    MoveAircraftCommand(AircraftItem* item, QPointF before, QPointF after)
        : m_item(item), m_before(before), m_after(after) {}
    void execute() override { m_item->setPos(m_after); }
    void undo()    override { m_item->setPos(m_before); }
    std::string describe() const override { return "Move Aircraft"; }
private:
    AircraftItem* m_item;
    QPointF m_before, m_after;
};

class RotateAircraftCommand : public arld::core::ICommand {
public:
    RotateAircraftCommand(AircraftItem* item, double before, double after)
        : m_item(item), m_before(before), m_after(after) {}
    void execute() override { m_item->setRotation(m_after); }
    void undo()    override { m_item->setRotation(m_before); }
    std::string describe() const override { return "Rotate Aircraft"; }
private:
    AircraftItem* m_item;
    double m_before, m_after;
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// AircraftItem
// ---------------------------------------------------------------------------
AircraftItem::AircraftItem(const arld::core::AircraftLibraryEntry& entry,
                           const QString& svgResourcePath,
                           QGraphicsItem* parent)
    : QGraphicsItemGroup(parent)
    , m_entry(entry)
    , m_displayType(entry.defaultDisplayType)
    , m_placementId(arld::core::ProjectFile::generateUuid()) {
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setCursor(Qt::SizeAllCursor);

    // SVG silhouette scaled so 1 scene unit = 1 foot.
    m_svgItem = new QGraphicsSvgItem(svgResourcePath, this);
    const QSizeF svgSize = m_svgItem->boundingRect().size();
    if (svgSize.width() > 0 && m_entry.wingspanFt > 0) {
        const double scaleF = m_entry.wingspanFt / svgSize.width();
        m_svgItem->setScale(scaleF);
    }

    // Record local centre before adding non-content children.
    const QRectF br = childrenBoundingRect();
    m_localCenter = br.center();
    setTransformOriginPoint(m_localCenter);

    // LOD placeholder rect (hidden by default; shown when simplified).
    m_lodRect = new QGraphicsRectItem(this);
    m_lodRect->setZValue(0.1);
    m_lodRect->setAcceptedMouseButtons(Qt::NoButton);
    m_lodRect->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_lodRect->setFlag(QGraphicsItem::ItemIsMovable, false);
    m_lodRect->setVisible(false);

    // Tail-swing arc — rendered below clearance zone.
    m_tailSwingItem = new QGraphicsPolygonItem(this);
    m_tailSwingItem->setZValue(-0.6);
    m_tailSwingItem->setAcceptedMouseButtons(Qt::NoButton);
    m_tailSwingItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_tailSwingItem->setFlag(QGraphicsItem::ItemIsMovable, false);
    {
        const QColor tailSwingFill(0xDD, 0xAA, 0x00,
                                   static_cast<int>(0.20f * 255));
        const QColor tailSwingOutline(0xBB, 0x88, 0x00, 200);
        m_tailSwingItem->setBrush(QBrush(tailSwingFill));
        m_tailSwingItem->setPen(QPen(tailSwingOutline, 0.5, Qt::DashLine));
    }
    // Only visible when minTurnRadiusFt is set.
    m_tailSwingItem->setVisible(m_entry.minTurnRadiusFt.has_value());

    // Clearance zone — rendered behind the silhouette.
    m_clearanceItem = new ClearanceZoneItem(this);
    m_clearanceItem->setDisplayType(m_displayType);
    rebuildClearancePolygon();

    // No-smoking circle for JET-A aircraft in Hot Ramp mode (Sprint 2-3-3)
    m_noSmokingItem = new QGraphicsEllipseItem(this);
    m_noSmokingItem->setZValue(-0.3);
    m_noSmokingItem->setPen(QPen(QColor("#CC2222"), 1.5, Qt::DashLine));
    m_noSmokingItem->setBrush(QBrush(QColor(204, 34, 34, 25)));
    m_noSmokingItem->setAcceptedMouseButtons(Qt::NoButton);
    m_noSmokingItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_noSmokingItem->setFlag(QGraphicsItem::ItemIsMovable, false);
    m_noSmokingItem->setVisible(false);

    // Aircraft label (Sprint 2-3-4)
    m_labelItem = new QGraphicsTextItem(this);
    m_labelItem->setZValue(1.5);
    m_labelItem->setAcceptedMouseButtons(Qt::NoButton);
    m_labelItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_labelItem->setFlag(QGraphicsItem::ItemIsMovable, false);
    {
        QFont lf = m_labelItem->font();
        lf.setPointSize(6);
        m_labelItem->setFont(lf);
    }
    m_labelItem->setDefaultTextColor(QColor(0x22, 0x22, 0x22));

    // Rotation handle — hidden until selected.
    auto* handle = new RotationHandle(this);
    handle->setVisible(false);
    Q_UNUSED(handle);

    // Accessibility: set tool tip as accessible name.
    updateAccessibleName();

    // Initialize label and no-smoking overlay.
    updateLabel();
    updateNoSmoking();
}

void AircraftItem::rebuildClearancePolygon() {
    if (!m_clearanceItem) return;

    const double baseMargin = static_cast<double>(
        arld::core::ClearanceEngine::requiredClearanceFt(m_displayType, m_entry.propArcFt));
    const double gearBonus  = m_gearExtended
        ? static_cast<double>(arld::core::kGearExtendedAdditionFt)
        : 0.0;
    const double margin = baseMargin + gearBonus;

    const double cx = m_localCenter.x();
    const double cy = m_localCenter.y();

    // --- Display-type-specific visual shapes ---
    QPolygonF poly;
    if (m_displayType == arld::core::DisplayType::TaxiOnly) {
        // Taxi-Only corridor: elongated along rotation axis, narrower on sides.
        const double hw = m_entry.wingspanFt / 2.0 + 10.0;
        const double hh = m_entry.lengthFt   / 2.0 + 50.0;
        poly << QPointF(cx - hw, cy - hh)
             << QPointF(cx + hw, cy - hh)
             << QPointF(cx + hw, cy + hh)
             << QPointF(cx - hw, cy + hh);
    } else {
        // Standard rectangle (all other display types).
        double hw = m_entry.wingspanFt / 2.0 + margin;
        double hh = m_entry.lengthFt   / 2.0 + margin;
        // If tail-swing extends further than normal envelope rear, expand.
        if (m_entry.minTurnRadiusFt.has_value()) {
            const double tailR = static_cast<double>(*m_entry.minTurnRadiusFt);
            if (tailR > hh) hh = tailR;
        }
        poly << QPointF(cx - hw, cy - hh)
             << QPointF(cx + hw, cy - hh)
             << QPointF(cx + hw, cy + hh)
             << QPointF(cx - hw, cy + hh);
    }

    m_clearanceItem->setPolygon(poly);
    m_clearanceItem->setDisplayType(m_displayType);

    // --- Tail-swing arc polygon ---
    if (m_tailSwingItem) {
        if (m_entry.minTurnRadiusFt.has_value()) {
            // Build the tail-swing polygon using the engine.
            const auto state = toAircraftState();
            const auto cgalPoly = arld::core::ClearanceEngine::tailSwingPolygon(state);

            if (cgalPoly.size() > 0) {
                // Convert from scene coordinates to local item coordinates.
                const QPointF scenePos = this->pos();
                QPolygonF localPoly;
                localPoly.reserve(static_cast<qsizetype>(cgalPoly.size()));
                for (std::size_t i = 0; i < cgalPoly.size(); ++i) {
                    const double sx = CGAL::to_double(cgalPoly.vertex(i).x());
                    const double sy = CGAL::to_double(cgalPoly.vertex(i).y());
                    // Map scene → local by subtracting item position.
                    localPoly << QPointF(sx - scenePos.x(), sy - scenePos.y());
                }
                m_tailSwingItem->setPolygon(localPoly);
                m_tailSwingItem->setVisible(true);
            } else {
                m_tailSwingItem->setVisible(false);
            }
        } else {
            m_tailSwingItem->setVisible(false);
        }
    }
}

arld::core::AircraftState AircraftItem::toAircraftState() const {
    arld::core::AircraftState s;
    s.id              = m_placementId;   // use placement ID so pairs are unique
    s.wingspanFt      = m_entry.wingspanFt;
    s.lengthFt        = m_entry.lengthFt;
    s.rotationDeg     = static_cast<float>(rotation());
    s.displayType     = m_displayType;
    s.propArcFt       = m_entry.propArcFt;
    s.minTurnRadiusFt = m_entry.minTurnRadiusFt;
    s.gearExtended    = m_gearExtended;

    // Aircraft centre in scene coordinates.
    const QPointF sceneCentre = mapToScene(m_localCenter);
    s.centerX = static_cast<float>(sceneCentre.x());
    s.centerY = static_cast<float>(sceneCentre.y());
    return s;
}

QVariant AircraftItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemSelectedChange) {
        for (auto* child : childItems()) {
            if (auto* handle = dynamic_cast<RotationHandle*>(child))
                handle->setVisible(value.toBool());
        }
    }
    return QGraphicsItemGroup::itemChange(change, value);
}

void AircraftItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    m_dragStartPos = pos();
    QGraphicsItemGroup::mousePressEvent(event);
}

void AircraftItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItemGroup::mouseReleaseEvent(event);
    const QPointF endPos = pos();
    if ((endPos - m_dragStartPos).manhattanLength() > 0.01 && onCommandReady) {
        struct AlreadyExecuted : arld::core::ICommand {
            std::unique_ptr<arld::core::ICommand> inner;
            bool done = false;
            explicit AlreadyExecuted(std::unique_ptr<arld::core::ICommand> c)
                : inner(std::move(c)) {}
            void execute() override { if (done) inner->execute(); done = true; }
            void undo()    override { inner->undo(); }
            std::string describe() const override { return inner->describe(); }
        };
        onCommandReady(std::make_unique<AlreadyExecuted>(
            std::make_unique<MoveAircraftCommand>(this, m_dragStartPos, endPos)));
    }
}

void AircraftItem::setDisplayType(arld::core::DisplayType dt) {
    m_displayType = dt;
    rebuildClearancePolygon();
    updateAccessibleName();
    updateNoSmoking();
    update();
}

void AircraftItem::setTailNumber(const std::string& s) {
    m_tailNumber = s;
    updateAccessibleName();
    updateLabel();
}

void AircraftItem::setOwner(const std::string& s) {
    m_owner = s;
    updateAccessibleName();
}

void AircraftItem::setFuelType(const std::string& s) {
    m_fuelType = s;
    updateNoSmoking();
}

void AircraftItem::setGearExtended(bool v) {
    if (m_gearExtended == v) return;
    m_gearExtended = v;
    rebuildClearancePolygon();
    update();
}

void AircraftItem::setHazmat(bool v) {
    m_hasHazmat = v;
    if (v) {
        if (!m_hazmatIcon) {
            m_hazmatIcon = new QGraphicsTextItem(QStringLiteral("⚠"), this);
            QFont f = m_hazmatIcon->font();
            f.setPointSize(8);
            f.setBold(true);
            m_hazmatIcon->setFont(f);
            m_hazmatIcon->setDefaultTextColor(QColor(0xCC, 0x22, 0x22));
            m_hazmatIcon->setZValue(1.0);
        }
        // Position at top-centre of bounding rect
        const QRectF br = childrenBoundingRect();
        const double iconW = m_hazmatIcon->boundingRect().width();
        m_hazmatIcon->setPos(br.center().x() - iconW / 2.0, br.top());
        m_hazmatIcon->setVisible(true);
    } else {
        if (m_hazmatIcon) {
            m_hazmatIcon->setVisible(false);
        }
    }
    update();
}

// ---------------------------------------------------------------------------
// LOD simplified rendering (Sprint 2-3-2)
// ---------------------------------------------------------------------------
void AircraftItem::setLodSimplified(bool simplified) {
    if (m_lodSimplified == simplified) return;
    m_lodSimplified = simplified;

    if (m_svgItem)  m_svgItem->setVisible(!simplified);
    if (m_lodRect) {
        if (simplified) {
            // Position the rect to cover the aircraft footprint (wingspan x length),
            // centred on m_localCenter.
            const double hw = m_entry.wingspanFt / 2.0;
            const double hl = m_entry.lengthFt   / 2.0;
            m_lodRect->setRect(m_localCenter.x() - hw, m_localCenter.y() - hl,
                               m_entry.wingspanFt,     m_entry.lengthFt);

            // Color by display type (same palette as SvgExporter)
            QColor fillColor;
            using DT = arld::core::DisplayType;
            switch (m_displayType) {
                case DT::StaticDisplay:      fillColor = QColor(0x44, 0x77, 0xAA); break;
                case DT::WarbirdHeritage:    fillColor = QColor(0x44, 0x77, 0x44); break;
                case DT::TaxiOnly:           fillColor = QColor(0x77, 0x77, 0xAA); break;
                case DT::MilitaryStatic:     fillColor = QColor(0xAA, 0x44, 0x44); break;
                case DT::HotRamp:            fillColor = QColor(0xCC, 0x77, 0x22); break;
                case DT::MediaPhotoPlatform: fillColor = QColor(0x88, 0x44, 0x88); break;
                case DT::RampShow:           fillColor = QColor(0x44, 0xAA, 0xAA); break;
            }
            fillColor.setAlpha(static_cast<int>(0.85 * 255));
            m_lodRect->setBrush(QBrush(fillColor));
            m_lodRect->setPen(QPen(fillColor.darker(150), 0.5));
        }
        m_lodRect->setVisible(simplified);
    }
}

// ---------------------------------------------------------------------------
// Label mode (Sprint 2-3-4)
// ---------------------------------------------------------------------------
void AircraftItem::updateLabel() {
    if (!m_labelItem) return;
    switch (m_labelMode) {
        case LabelMode::DisplayName:
            m_labelItem->setPlainText(QString::fromStdString(m_entry.displayName));
            m_labelItem->setVisible(true);
            break;
        case LabelMode::TailNumber: {
            const QString tn = QString::fromStdString(m_tailNumber);
            m_labelItem->setPlainText(tn.isEmpty()
                ? QString::fromStdString(m_entry.displayName)
                : tn);
            m_labelItem->setVisible(true);
            break;
        }
        case LabelMode::Hidden:
            m_labelItem->setVisible(false);
            break;
    }
    // Re-center label above the aircraft
    if (m_labelItem->isVisible()) {
        const QRectF lblBr = m_labelItem->boundingRect();
        m_labelItem->setPos(m_localCenter.x() - lblBr.width() / 2.0,
                            m_localCenter.y() - m_entry.lengthFt / 2.0 - lblBr.height() - 2.0);
    }
}

void AircraftItem::setLabelMode(LabelMode mode) {
    if (m_labelMode == mode) return;
    m_labelMode = mode;
    updateLabel();
}

// ---------------------------------------------------------------------------
// No-smoking overlay (Sprint 2-3-3)
// ---------------------------------------------------------------------------
void AircraftItem::updateNoSmoking() {
    if (!m_noSmokingItem) return;
    const bool hotRamp = (m_displayType == arld::core::DisplayType::HotRamp);
    const bool jetFuel = (m_fuelType.find("JET") != std::string::npos ||
                          m_fuelType.find("Jet") != std::string::npos ||
                          m_fuelType.find("jet") != std::string::npos);
    const bool show = hotRamp && jetFuel;
    constexpr float kNoSmokingRadiusFt = 100.0f;
    if (show) {
        m_noSmokingItem->setRect(
            m_localCenter.x() - kNoSmokingRadiusFt,
            m_localCenter.y() - kNoSmokingRadiusFt,
            kNoSmokingRadiusFt * 2,
            kNoSmokingRadiusFt * 2);
    }
    m_noSmokingItem->setVisible(show);
}

void AircraftItem::updateAccessibleName() {
    // Build a human-readable accessible name for screen readers and VoiceOver.
    // Format: "TAIL_NUMBER (DisplayType)" or "DisplayName (DisplayType)" if no tail.
    auto dtToStr = [](arld::core::DisplayType dt) -> QString {
        using DT = arld::core::DisplayType;
        switch (dt) {
            case DT::StaticDisplay:      return QStringLiteral("Static Display");
            case DT::WarbirdHeritage:    return QStringLiteral("Warbird/Heritage");
            case DT::TaxiOnly:           return QStringLiteral("Taxi Only");
            case DT::MilitaryStatic:     return QStringLiteral("Military Static");
            case DT::HotRamp:            return QStringLiteral("Hot Ramp");
            case DT::MediaPhotoPlatform: return QStringLiteral("Media/Photo Platform");
            case DT::RampShow:           return QStringLiteral("Ramp Show");
        }
        return QStringLiteral("Unknown");
    };

    const QString typeStr = dtToStr(m_displayType);
    const QString tail = QString::fromStdString(m_tailNumber);
    const QString name = tail.isEmpty()
        ? QStringLiteral("%1 (%2)").arg(
              QString::fromStdString(m_entry.displayName), typeStr)
        : QStringLiteral("%1 (%2)").arg(tail, typeStr);

    // Qt QGraphicsItem uses toolTip for accessibility on macOS VoiceOver.
    setToolTip(name);
}

void AircraftItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    auto addLabelAction = [&](const QString& text, LabelMode mode) {
        QAction* act = menu.addAction(text);
        act->setCheckable(true);
        act->setChecked(m_labelMode == mode);
        QObject::connect(act, &QAction::triggered, [this, mode] {
            setLabelMode(mode);
        });
    };
    addLabelAction(QStringLiteral("Label: Display Name"), LabelMode::DisplayName);
    addLabelAction(QStringLiteral("Label: Tail Number"),  LabelMode::TailNumber);
    addLabelAction(QStringLiteral("Label: Hidden"),       LabelMode::Hidden);
    menu.exec(event->screenPos());
    event->accept();
}

void AircraftItem::applyRotation(double angleDeg) {
    setRotation(angleDeg);
}

void AircraftItem::finishRotation(double fromDeg, double toDeg) {
    if (onCommandReady) {
        struct AlreadyExecuted : arld::core::ICommand {
            std::unique_ptr<arld::core::ICommand> inner;
            bool done = false;
            explicit AlreadyExecuted(std::unique_ptr<arld::core::ICommand> c)
                : inner(std::move(c)) {}
            void execute() override { if (done) inner->execute(); done = true; }
            void undo()    override { inner->undo(); }
            std::string describe() const override { return inner->describe(); }
        };
        onCommandReady(std::make_unique<AlreadyExecuted>(
            std::make_unique<RotateAircraftCommand>(this, fromDeg, toDeg)));
    }
}

void AircraftItem::commitRotation(double fromDeg, double toDeg) {
    finishRotation(fromDeg, toDeg);
}

} // namespace arld::ui
