# Changelog

All notable changes to ARLD are documented here.

## Phase 2 — Feature Complete (Months 7–12)

### Sprint 2-6 · 2026-05-12
- Fix: satellite underlay image now correctly clears when File → New is used (issue #15)
- Fix: `bench_library_load` benchmark test data corrected (`GENERAL_AVIATION` category)
- Accessibility: `RampView` canvas exposes accessible name and description for screen readers
- Accessibility: dock panel widgets expose accessible names
- Docs: `SECURITY_REVIEW.md`, `PHASE3_GATE.md`, accessibility audit update

### Sprint 2-5 · 2026-05-12
- Scale bar variants: `ExportOptions::ScaleBarMode` — `ImperialOnly`, `MetricOnly`, `Dual`
- PNG exporter: embedded 5×7 bitmap font for scale bar text labels
- PDF exporter: scale bar drawn via libharu above the title block
- `LICENSES.txt`: SPDX identifier table for all third-party dependencies

### Sprint 2-4 · 2026-05-11
- `MacroCommand` + `UndoStack::beginMacro`/`endMacro` for atomic multi-step undo
- Snap-to-heading batch undo via macros; arrow-key nudge (1 ft / Shift=5 ft)
- macOS dock tile violation badge via native ObjC `AppBadge`
- Lazy SVG silhouette loading on project open (deferred via `QTimer::singleShot`)
- `LibraryUpdateChecker::downloadCompleted` signal; `LibraryPanel::reloadLibrary()` auto-reload
- `SatelliteUnderlayItem::setGeoreference`: Web Mercator GSD with lat/lon/zoom dialog inputs
- Fix: satellite underlay georeferenced scaling (issue #14)

### Sprint 2-3 · 2026-05-10
- LOD rendering: aircraft render as colored rects at scale > 1:2000 for 60 fps with 200+ aircraft
- Hot Ramp JET-A no-smoking 100 ft circle overlay
- Arrival/departure time fields on each placed aircraft (persisted in `.arld`)
- Right-click label toggle: Display Name / Tail Number / Hidden
- CVD accessibility: hatch patterns on clearance zones (diagonal, cross-hatch, horizontal)
- JSON depth limit: files with nesting > 32 levels rejected at load time
- Library update checker: HTTPS manifest fetch, incremental download, auto-reload
- `NOTICES.txt` generation target; About dialog with View Licenses button

### Sprint 2-2 · 2026-05-09
- Named layout snapshots (`versions[]` in `.arld`); switch, export, compare versions
- Visual change-delta overlay: Added (green), Removed (red ghost), Moved (amber)
- Minimap panel: 200×150 px scene overview, click to pan
- Undo history panel: last 20 commands, click to jump
- Satellite tile streaming: Mapbox HTTPS fetch with georeferenced GSD
- KML/GeoJSON boundary import (flat-earth projection, centroid at origin)
- Tag-triggered CPack packaging; GitHub Release automation

### Sprint 2-1 · 2026-05-08
- Aircraft library expanded to 150 entries (all categories)
- `BatchExporter::exportAll` — SVG, PDF, PNG, JPEG in one action
- SVG named layers (`inkscape:label`) for layer-aware editing
- Scale bar overlay in PNG export
- Library browser: sort by Name, Wingspan, Length
- `AircraftManifestExporter` CSV with placement metadata
- Rubber-band lasso multi-select; group move as single undoable command
- Community submission button (opens GitHub issue template)
- Export performance benchmarks

---

## Phase 1 — Production Desktop (Months 3–6) — ✅ Complete

Sprints 1-1 through 1-6: PoC-to-production refactor; CPack packaging; UnitConverter; auto-save; violations panel; clearance rule config; library browser; custom aircraft; tail-swing; gear states; accessibility foundation; PDF/PNG/JPEG export; satellite underlay; violations CSV report.

## Phase 0 — C++ PoC (Weeks 1–10) — ✅ Complete

Sprints 0-1 through 0-5: CMake/vcpkg scaffold; Qt canvas with pan/zoom/undo; 20-aircraft library with SVG silhouettes; CGAL clearance zones; SVG export; `.arld` JSON project save/load.
