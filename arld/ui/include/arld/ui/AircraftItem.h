#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <arld/core/ClearanceEngine.h>
#include <QGraphicsItemGroup>
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
    ClearanceZoneItem* m_clearanceItem = nullptr;

    friend class RotationHandle;
    void applyRotation(double angleDeg);
    void finishRotation(double fromDeg, double toDeg);
};

} // namespace arld::ui
