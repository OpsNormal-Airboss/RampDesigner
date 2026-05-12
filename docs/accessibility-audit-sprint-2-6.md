# Accessibility Audit — Sprint 2-6

**Previous audit:** `accessibility-audit-sprint-1-4.md`  
**Date:** 2026-05-12

## New in Sprint 2-6

### Canvas Widget Accessible Name
- `RampView` now sets `accessibleName = "Ramp Design Canvas"` and `accessibleDescription`
- Screen readers announce the canvas when it receives keyboard focus

### Dock Panel Accessible Names
- Library, Properties, and Violations dock widgets have explicit `setAccessibleName()` calls
- Accessible names are distinct from window titles, improving NVDA/VoiceOver navigation

## Remaining Limitation

`QGraphicsItem` (used for `AircraftItem`, `RampBoundaryItem`, `ClearanceZoneItem`) does not integrate natively with `QAccessible` without a custom factory that maps items into the widget accessibility tree. This is a Qt framework limitation.

**Workaround for screen reader users:**
- The Violations panel lists all clearance violations with aircraft identifiers
- The Properties panel announces the selected aircraft's tail number, owner, and display type
- Keyboard shortcut `Ctrl+0` centers the view on the selected aircraft

**Deferred to Phase 3:** Implement `QAccessibleInterface` factory for `AircraftItem` that registers each placed aircraft as an accessible child of the `RampView` widget. This requires a dedicated accessibility sprint.

## WCAG 2.1 AA Compliance

| Category | Status |
|----------|--------|
| 1.4.3 Text Contrast (4.5:1) | ✅ All status bar and panel labels compliant |
| 1.4.11 Non-text Contrast (3:1) | ✅ CVD hatch patterns + color coding |
| 2.1.1 Keyboard navigable | ✅ Tab order set; canvas focusable |
| 2.4.6 Headings and Labels | ✅ All panels have accessible names |
| 4.1.2 Name, Role, Value | ⚠️ Canvas items not individually addressable (Qt limitation) |
