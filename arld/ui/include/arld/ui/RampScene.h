#pragma once
#include <arld/core/UndoStack.h>
#include <arld/core/AircraftLibraryEntry.h>
#include <QGraphicsScene>
#include <QTimer>
#include <vector>

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

    void placeAircraft(const arld::core::AircraftLibraryEntry& entry, QPointF scenePos);

    // Called by RampView on every zoom change.
    void updateOverlay(double pixelsPerFt, QPointF scaleBarScenePos);

    // Trigger an immediate clearance recomputation (normally debounced via timer).
    void recomputeClearance();

signals:
    void editModeChanged(EditMode mode);
    // Emitted after each clearance evaluation; count = number of violation pairs.
    void violationCountChanged(int count);

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

    std::vector<AircraftItem*> m_aircraft;  // all placed (possibly hidden) aircraft
    QTimer m_clearanceTimer;                // debounce: fires 80 ms after last scene change
};

} // namespace arld::ui
