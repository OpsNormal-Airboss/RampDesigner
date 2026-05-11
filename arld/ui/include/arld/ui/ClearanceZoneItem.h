#pragma once
#include <arld/core/ClearanceEngine.h>
#include <QGraphicsPolygonItem>

namespace arld::ui {

// Renders the clearance envelope for one aircraft as a child of AircraftItem.
// The polygon is expressed in local parent-item coordinates, so it rotates
// automatically with the parent without needing scene-space conversion.
// Color reflects violation status: green (clear) / yellow (advisory) / red (violation).
class ClearanceZoneItem : public QGraphicsPolygonItem {
public:
    explicit ClearanceZoneItem(QGraphicsItem* parent = nullptr);

    void setStatus(arld::core::ClearanceSeverity status);
    arld::core::ClearanceSeverity status() const { return m_status; }

private:
    void updateAppearance();

    arld::core::ClearanceSeverity m_status = arld::core::ClearanceSeverity::Clear;
};

} // namespace arld::ui
