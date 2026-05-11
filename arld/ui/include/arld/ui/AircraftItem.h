#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <QGraphicsItemGroup>
#include <functional>
#include <memory>

namespace arld::core { class ICommand; }

namespace arld::ui {

class RotationHandle;

class AircraftItem : public QGraphicsItemGroup {
public:
    AircraftItem(const arld::core::AircraftLibraryEntry& entry,
                 const QString& svgResourcePath,
                 QGraphicsItem* parent = nullptr);

    const arld::core::AircraftLibraryEntry& entry() const { return m_entry; }

    // Called by RampScene when the item is placed or moved via undo/redo.
    void setDisplayType(arld::core::DisplayType dt);
    arld::core::DisplayType displayType() const { return m_displayType; }

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    // Callback fired when an undoable action is ready (move, rotate).
    std::function<void(std::unique_ptr<arld::core::ICommand>)> onCommandReady;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    arld::core::AircraftLibraryEntry m_entry;
    arld::core::DisplayType m_displayType;
    QPointF m_dragStartPos;

    friend class RotationHandle;
    void applyRotation(double angleDeg);
    void finishRotation(double fromDeg, double toDeg);
};

} // namespace arld::ui
