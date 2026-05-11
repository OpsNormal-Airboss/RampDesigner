#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <arld/core/ClearanceEngine.h>
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
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
                 QGraphicsItem* parent = nullptr);

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

    // Per-aircraft metadata (Sprint 1-2-5 / 1-2-7)
    const std::string& tailNumber() const { return m_tailNumber; }
    void setTailNumber(const std::string& s) { m_tailNumber = s; }

    const std::string& owner() const { return m_owner; }
    void setOwner(const std::string& s) { m_owner = s; }

    const std::string& fuelType() const { return m_fuelType; }
    void setFuelType(const std::string& s) { m_fuelType = s; }

    bool hasHazmat() const { return m_hasHazmat; }
    void setHazmat(bool v);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void rebuildClearancePolygon();

    arld::core::AircraftLibraryEntry m_entry;
    arld::core::DisplayType m_displayType;
    std::string m_placementId;
    QPointF m_localCenter;     // bounding rect centre in local item coords
    QPointF m_dragStartPos;
    ClearanceZoneItem*  m_clearanceItem = nullptr;
    QGraphicsTextItem*  m_hazmatIcon    = nullptr;

    // Per-aircraft metadata
    std::string m_tailNumber;
    std::string m_owner;
    std::string m_fuelType;
    bool        m_hasHazmat = false;

    friend class RotationHandle;
    void applyRotation(double angleDeg);
    void finishRotation(double fromDeg, double toDeg);
};

} // namespace arld::ui
