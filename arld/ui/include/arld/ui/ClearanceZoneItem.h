#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <arld/core/ClearanceEngine.h>
#include <QGraphicsPolygonItem>

namespace arld::ui {

// Renders the clearance envelope for one aircraft as a child of AircraftItem.
// The polygon is expressed in local parent-item coordinates, so it rotates
// automatically with the parent without needing scene-space conversion.
// Color reflects violation status: green (clear) / yellow (advisory) / red (violation).
// WCAG/CVD: pattern fills augment color to aid colorblind users.
class ClearanceZoneItem : public QGraphicsPolygonItem {
public:
    explicit ClearanceZoneItem(QGraphicsItem* parent = nullptr);

    void setStatus(arld::core::ClearanceSeverity status);
    arld::core::ClearanceSeverity status() const { return m_status; }

    // Display-type-aware pen/brush styling (1-4-3, 1-4-5).
    void setDisplayType(arld::core::DisplayType dt);
    arld::core::DisplayType displayType() const { return m_displayType; }

private:
    void updateAppearance();

    arld::core::ClearanceSeverity m_status      = arld::core::ClearanceSeverity::Clear;
    arld::core::DisplayType       m_displayType = arld::core::DisplayType::StaticDisplay;
};

} // namespace arld::ui
