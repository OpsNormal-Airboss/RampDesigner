#pragma once
#include <arld/core/ClearanceEngine.h>
#include <arld/core/ClearanceRuleSet.h>
#include <arld/core/LayoutDiffer.h>
#include <arld/core/ProjectFile.h>
#include <arld/core/UndoStack.h>
#include <arld/core/AircraftLibraryEntry.h>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QTimer>
#include <functional>
#include <vector>

namespace arld::ui {

class AircraftItem;
class GridOverlayItem;
class RampBoundaryItem;
class SatelliteUnderlayItem;
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

    /// Move all currently-selected aircraft by (dx, dy) feet as a single undoable step.
    void nudgeSelected(double dx, double dy);

    // Called by RampView on every zoom change.
    void updateOverlay(double pixelsPerFt, QPointF scaleBarScenePos);

    // --- Grid overlay (1-3-7) ---
    void setGridVisible(bool visible);
    void setGridSpacingFt(int spacingFt);

    // --- Satellite underlay (1-5-6) ---
    void setSatelliteImage(const QString& path);
    void setSatelliteOpacity(float opacity);
    SatelliteUnderlayItem* satelliteItem() const { return m_satelliteItem; }

    // --- Delta overlay (Sprint 2-2) ---
    void showDelta(const arld::core::DeltaResult& delta);
    void clearDelta();

    // --- Boundary import (Sprint 2-2) ---
    void setBoundary(const arld::core::RampBoundaryData& boundary);

    // Trigger an immediate clearance recomputation (normally debounced via timer).
    void recomputeClearance();

    // --- ClearanceRuleSet (Sprint 1-2-2) ---
    const arld::core::ClearanceRuleSet& ruleSet() const { return m_ruleSet; }
    void setRuleSet(const arld::core::ClearanceRuleSet& rs);

    // --- Violation results accessor (Sprint 1-2-1) ---
    const std::vector<arld::core::ViolationResult>& lastViolations() const { return m_lastViolations; }

    // --- Override management (Sprint 1-2-6) ---
    void addOverride(const arld::core::ClearanceOverride& ov);
    const std::vector<arld::core::ClearanceOverride>& overrides() const { return m_overrides; }

    // --- ViolationsPanel helper (Sprint 1-2-1) ---
    QPointF aircraftSceneCenter(const std::string& placementId) const;

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
    // Emitted whenever violations change (Sprint 1-2-1).
    void violationsChanged();
    // Emitted after each clearance evaluation; count = number of visible placed aircraft.
    void aircraftCountChanged(int count);
    // Emitted whenever the scene is dirtied (aircraft moved, placed, boundary edited, etc.).
    void sceneModified();
    // Emitted when the user requests the Properties panel for a specific aircraft.
    void propertiesRequested(arld::ui::AircraftItem* item);
    // Emitted when the active clearance ruleset changes (Sprint 3-2-9).
    void ruleSetChanged(const QString& displayName);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QPointF snapToGrid(QPointF pos, bool freehand) const;

    EditMode m_mode = EditMode::Select;
    arld::core::UndoStack m_undoStack;
    RampBoundaryItem*          m_boundaryItem  = nullptr;
    ScaleBarItem*              m_scaleBarItem  = nullptr;
    GridOverlayItem*           m_gridOverlay   = nullptr;
    SatelliteUnderlayItem*     m_satelliteItem = nullptr;
    QString                    m_satelliteImagePath;

    std::vector<AircraftItem*>  m_aircraft;       // all placed (possibly hidden) aircraft
    QTimer m_clearanceTimer;                       // debounce: fires 80 ms after last scene change

    arld::core::ClearanceRuleSet                    m_ruleSet;
    std::vector<arld::core::ViolationResult>        m_lastViolations;
    std::vector<arld::core::ClearanceOverride>      m_overrides;

    // Multi-select drag: pre-drag positions of all selected aircraft.
    std::vector<std::pair<AircraftItem*, QPointF>> m_preDragPositions;

    // Delta mode state (Sprint 2-2)
    bool m_deltaMode = false;
    std::vector<QGraphicsRectItem*> m_deltaGhosts;  // ghost items for removed/moved aircraft
};

} // namespace arld::ui
