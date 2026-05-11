# Accessibility Audit — Sprint 1-4

## Audit Method
- Qt Accessibility Inspector (macOS Accessibility Inspector app)
- Manual Tab-key traversal through all panels and toolbars
- VoiceOver (macOS) spot-check on canvas items and dock widgets

## Findings

### Keyboard Navigation
- Tab order set via `QWidget::setTabOrder` in `MainWindow` constructor:
  `LibraryPanel → PropertiesPanel → ViolationsPanel`
- Each dock widget exposes a `focusProxy()` pointing to its primary interactive widget
- Arrow-key navigation works within the violations table and aircraft list

### Text Contrast
- WCAG 2.1 AA compliant for all status bar and panel labels (contrast ratio ≥ 4.5:1)
- Clearance zone overlays use semi-transparent fills; underlying content remains readable

### CVD Pattern Fills (Color-Vision Deficiency)
- Clear zone: solid green (existing)
- Advisory zone: `Qt::BDiagPattern` diagonal lines on amber
- Violation zone: `Qt::DiagCrossPattern` cross-hatch on red
- Overridden zone: `Qt::HorPattern` horizontal lines on orange
- Users can distinguish zones by texture and position, not just color

### Accessible Names on QGraphicsItem
- `AircraftItem` sets `toolTip()` to `"TAIL_NUMBER (DisplayType)"` for VoiceOver
- Updated on `setTailNumber()`, `setOwner()`, `setDisplayType()`
- `RampBoundaryItem`: inherits default Qt accessibility from parent scene

## Known Limitations
- `QGraphicsItem` does not integrate natively with `QAccessible` without a custom
  `QAccessibleInterface` factory — deferred to Phase 2
- Focus rectangle not visible on canvas items (Qt limitation for QGraphicsView)
- Screen reader announcements on item drag/drop are not yet implemented
