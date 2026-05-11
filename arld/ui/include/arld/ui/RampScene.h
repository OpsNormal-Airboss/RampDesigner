#pragma once
#include <arld/core/UndoStack.h>
#include <arld/core/AircraftLibraryEntry.h>
#include <QGraphicsScene>

namespace arld::ui {

class AircraftItem;
class RampBoundaryItem;
class ScaleBarItem;

enum class EditMode { Select, DrawBoundary };

class RampScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit RampScene(QObject* parent = nullptr);

    void setEditMode(EditMode mode);
    EditMode editMode() const { return m_mode; }

    arld::core::UndoStack& undoStack() { return m_undoStack; }

    // Place an aircraft from the library at the given scene position.
    // The position is the aircraft's geometric centre.
    void placeAircraft(const arld::core::AircraftLibraryEntry& entry, QPointF scenePos);

    // Called by RampView on every zoom change.
    void updateOverlay(double pixelsPerFt, QPointF scaleBarScenePos);

signals:
    void editModeChanged(EditMode mode);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QPointF snapToGrid(QPointF pos, bool freehand) const;

    EditMode m_mode = EditMode::Select;
    arld::core::UndoStack m_undoStack;
    RampBoundaryItem* m_boundaryItem = nullptr;
    ScaleBarItem* m_scaleBarItem = nullptr;
};

} // namespace arld::ui
