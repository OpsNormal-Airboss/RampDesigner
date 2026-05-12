#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <arld/core/ClearanceEngine.h>
#include <arld/core/ProjectFile.h>
#include <QFutureWatcher>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsSvgItem>
#include <functional>
#include <memory>
#include <string>

namespace arld::core { class ICommand; }

namespace arld::ui {

class ClearanceZoneItem;
class RotationHandle;

class AircraftItem : public QGraphicsItemGroup {
public:
    AircraftItem(const arld::core::AircraftLibraryEntry& entry,
                 const QString& svgResourcePath,
                 QGraphicsItem* parent = nullptr,
                 bool lazy = false);

    const arld::core::AircraftLibraryEntry& entry() const { return m_entry; }

    // Unique placement identifier (UUID v4), assigned at construction.
    std::string placementId() const { return m_placementId; }
    void setPlacementId(const std::string& id) { m_placementId = id; }

    void setDisplayType(arld::core::DisplayType dt);
    arld::core::DisplayType displayType() const { return m_displayType; }

    ClearanceZoneItem* clearanceItem() { return m_clearanceItem; }

    // Snapshot of this item's spatial state for clearance computation.
    arld::core::AircraftState toAircraftState() const;

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    // Callback fired when an undoable action is ready (move, rotate).
    std::function<void(std::unique_ptr<arld::core::ICommand>)> onCommandReady;
    // Callback fired when the user requests the Properties panel for this item.
    std::function<void()> onPropertiesRequested;

    /// Apply a rotation and push an undo command.
    /// fromDeg is the previous rotation; the item should already be at toDeg.
    void commitRotation(double fromDeg, double toDeg);

    /// Push a move undo command. The item must already be at @p to.
    void commitMove(QPointF from, QPointF to);

    // Per-aircraft metadata (Sprint 1-2-5 / 1-2-7)
    const std::string& tailNumber() const { return m_tailNumber; }
    void setTailNumber(const std::string& s);

    const std::string& owner() const { return m_owner; }
    void setOwner(const std::string& s);

    const std::string& fuelType() const { return m_fuelType; }
    void setFuelType(const std::string& s);

    const std::string& arrivalTime() const   { return m_arrivalTime; }
    void setArrivalTime(const std::string& t);
    const std::string& departureTime() const { return m_departureTime; }
    void setDepartureTime(const std::string& t);

    bool hasHazmat() const { return m_hasHazmat; }
    void setHazmat(bool v);

    // Gear state (Sprint 1-4-2)
    bool gearExtended() const { return m_gearExtended; }
    void setGearExtended(bool v);

    // LOD simplified rendering (Sprint 2-3-2)
    void setLodSimplified(bool simplified);
    bool isLodSimplified() const { return m_lodSimplified; }

    /// Defer SVG loading to a background thread (Sprint 2-4-7).
    /// Shows the LOD rect immediately; swaps to SVG renderer when loaded.
    void loadSvgDeferred(const QString& svgResourcePath);

    // Label mode (Sprint 2-3-4)
    using LabelMode = arld::core::PlacedAircraft::LabelMode;
    void setLabelMode(LabelMode mode);
    LabelMode labelMode() const { return m_labelMode; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    void rebuildClearancePolygon();
    void updateAccessibleName();
    void updateLabel();
    void updateNoSmoking();

    arld::core::AircraftLibraryEntry m_entry;
    arld::core::DisplayType m_displayType;
    std::string m_placementId;
    QPointF m_localCenter;     // bounding rect centre in local item coords
    QPointF m_dragStartPos;
    ClearanceZoneItem*       m_clearanceItem  = nullptr;
    QGraphicsPolygonItem*    m_tailSwingItem  = nullptr;
    QGraphicsTextItem*       m_hazmatIcon     = nullptr;
    QGraphicsTextItem*       m_labelItem      = nullptr;

    // LOD (Sprint 2-3-2)
    QGraphicsSvgItem*        m_svgItem        = nullptr;
    QGraphicsRectItem*       m_lodRect        = nullptr;
    bool                     m_lodSimplified  = false;

    // Rotation handle and state
    QGraphicsEllipseItem*    m_rotationHandle = nullptr;
    bool                     m_rotating       = false;
    double                   m_rotateStartAngle = 0.0;

    // No-smoking overlay (Sprint 2-3-3)
    QGraphicsEllipseItem*    m_noSmokingItem  = nullptr;

    // Per-aircraft metadata
    std::string m_tailNumber;
    std::string m_owner;
    std::string m_fuelType;
    std::string m_arrivalTime;
    std::string m_departureTime;
    bool        m_hasHazmat    = false;

    // Gear state (Sprint 1-4-2)
    bool        m_gearExtended = true;

    // Label mode (Sprint 2-3-4)
    LabelMode   m_labelMode = LabelMode::DisplayName;

    void finishRotation(double fromDeg, double toDeg);
};

} // namespace arld::ui
