#include <arld/ui/AircraftItem.h>
#include <arld/ui/ClearanceZoneItem.h>
#include <arld/core/ICommand.h>
#include <arld/core/ClearanceEngine.h>
#include <arld/core/ProjectFile.h>
#include <QGuiApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QCursor>
#include <QPolygonF>
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
    auto* svg = new QGraphicsSvgItem(svgResourcePath, this);
    const QSizeF svgSize = svg->boundingRect().size();
    if (svgSize.width() > 0 && m_entry.wingspanFt > 0) {
        const double scaleF = m_entry.wingspanFt / svgSize.width();
        svg->setScale(scaleF);
    }

    // Record local centre before adding non-content children.
    const QRectF br = childrenBoundingRect();
    m_localCenter = br.center();
    setTransformOriginPoint(m_localCenter);

    // Clearance zone — rendered behind the silhouette.
    m_clearanceItem = new ClearanceZoneItem(this);
    rebuildClearancePolygon();

    // Rotation handle — hidden until selected.
    auto* handle = new RotationHandle(this);
    handle->setVisible(false);
    Q_UNUSED(handle);
}

void AircraftItem::rebuildClearancePolygon() {
    if (!m_clearanceItem) return;

    const double margin = static_cast<double>(
        arld::core::ClearanceEngine::requiredClearanceFt(m_displayType, m_entry.propArcFt));

    const double hw = m_entry.wingspanFt / 2.0 + margin;
    const double hh = m_entry.lengthFt   / 2.0 + margin;
    const double cx = m_localCenter.x();
    const double cy = m_localCenter.y();

    QPolygonF poly;
    poly << QPointF(cx - hw, cy - hh)
         << QPointF(cx + hw, cy - hh)
         << QPointF(cx + hw, cy + hh)
         << QPointF(cx - hw, cy + hh);

    m_clearanceItem->setPolygon(poly);
}

arld::core::AircraftState AircraftItem::toAircraftState() const {
    arld::core::AircraftState s;
    s.id          = m_placementId;   // use placement ID so pairs are unique
    s.wingspanFt  = m_entry.wingspanFt;
    s.lengthFt    = m_entry.lengthFt;
    s.rotationDeg = static_cast<float>(rotation());
    s.displayType = m_displayType;
    s.propArcFt   = m_entry.propArcFt;

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

} // namespace arld::ui
