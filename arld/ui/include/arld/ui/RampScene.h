#pragma once
#include <arld/core/UndoStack.h>
#include <arld/core/AircraftLibraryEntry.h>
#include <arld/core/ProjectFile.h>
#include <QGraphicsScene>
#include <QTimer>
#include <functional>
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

    // --- Sprint 0-5: persistence ---

    /// Serialize the current scene state to a ProjectData snapshot.
    arld::core::ProjectData toProjectData() const;

    /// Restore scene state from @p data.
    /// @p lookup must return a pointer to the AircraftLibraryEntry for a given library id,
    /// or nullptr if the id is not found (that aircraft is then skipped).
    void loadProjectData(
        const arld::core::ProjectData& data,
        std::function<const arld::core::AircraftLibraryEntry*(const std::string&)> lookup);

    /// Remove all aircraft and boundary data, clear the undo stack.
    void clearScene();

signals:
    void editModeChanged(EditMode mode);
    // Emitted after each clearance evaluation; count = number of violation pairs.
    void violationCountChanged(int count);
    // Emitted whenever the scene is dirtied (aircraft moved, placed, boundary edited, etc.).
    void sceneModified();

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
