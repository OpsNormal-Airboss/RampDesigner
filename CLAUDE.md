<div align="center">
  <img src="https://raw.githubusercontent.com/OpsNormal-Airboss/CompanyTrademarks/main/Branding/GreenDotAirboss1Line.svg" alt="OpsNormal Airboss" width="320"/>
</div>

---

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 🗺️ Project

**Airshow Ramp Layout Designer (ARLD)** — a safety-critical C++ desktop application for designing, validating, and publishing aircraft parking layouts for static and flying airshows. It enforces FAA Certificate of Waiver (CoW) clearance rules in real time and exports print-quality diagrams.

**Current status:** Phase 0 (C++ PoC) — **All 5 sprints complete.** Build system, CI, Qt canvas, undo/redo, boundary drawing, 20-aircraft library, drag-and-drop placement, CGAL clearance engine, real-time violation detection, SVG export, and JSON project save/load are all implemented. Phase 0 PoC acceptance gate passed (34/34 tests, 15-aircraft round-trip verified). Phase 1 planning next.

## 📋 Post-Sprint Documentation

After completing each sprint, update the following files to reflect the current state of the project:

- **CLAUDE.md** — update commands, architecture, and any guidance that has changed
- **README.md** — update the getting started guide, feature summary, and setup instructions
- **RUNBOOK.md** — update operational procedures, deployment steps, and runbook entries for any new or changed functionality

## 📅 Phases

| Phase | Scope | Timeline |
|-------|-------|----------|
| 0 | C++ PoC — 20 aircraft, clearance engine, SVG export, JSON save/load | Weeks 1–10 (5 sprints) |
| 1 | Production desktop, 75 aircraft, all display types, satellite underlay | Months 3–6 |
| 2 | 150+ aircraft, full export suite, UAT | Months 7–12 |
| 3 | Public v1.0 release, open-source community edition, commercial tier | Months 13–14 |
| 4 | SaaS platform (TypeScript/React + AWS) — **pending steering committee approval** | Post Phase 3 |

## 🟢 Sprint Log (Phase 0)

| Sprint | Goal | Status |
|--------|------|--------|
| 0-1 | CMake/vcpkg scaffold, CI matrix, Config.h, smoke tests | ✅ Complete |
| 0-2 | Qt canvas prototype — pan/zoom, boundary drawing, undo/redo | ✅ Complete |
| 0-3 | 20-aircraft library, SVG silhouettes, drag-and-drop placement | ✅ Complete |
| 0-4 | CGAL clearance zones, real-time violation detection | ✅ Complete |
| 0-5 | SVG export, JSON project save/load, PoC acceptance gate | ✅ Complete |

## 🔧 Tech Stack

**Phases 0–3 (Desktop)**
- Language: C++20 (Clang 16+, GCC 13+, MSVC 2022)
- GUI/Canvas: Qt 6.7 LGPL — `QGraphicsScene`/`QGraphicsView`
- Geometry: CGAL 5.6 LGPL — rotated-rectangle polygon construction, squared-distance violation detection, spatial indexing
- JSON/Persistence: nlohmann/json 3.11
- PDF Export: libharu 2.4 (Phase 1+)
- Raster Export: stb_image_write header-only (Phase 1+)
- Build: CMake 3.28 + vcpkg manifest mode
- Testing: Catch2 3.x + CTest
- CI: GitHub Actions (macos-14 / ubuntu-22.04 / windows-2022 matrix)
- Packaging: CPack → `.dmg` / `.AppImage`+`.deb` / `.msi` (Phase 1+)

**Phase 4 (SaaS — separate phase)**
- Frontend: TypeScript + React, C++ geometry via WebAssembly or REST
- API: Node.js + Fastify on AWS ECS Fargate
- Auth: Auth0 / Stripe billing

## 🏗️ Module Architecture

Four strictly layered modules — higher layers depend on lower; no reverse dependencies, no circular imports:

```
Layer 1 — arld/core/      Geometry engine, clearance rules, aircraft library parser,
                           project file I/O, unit conversion.
                           Zero Qt dependency — must compile headless.

Layer 2 — arld/export/    SVG, PDF, PNG, JPEG exporters (strategy pattern via IExporter).
                           Depends on core/ only. (Scaffolded; implementation Sprint 0-5+)

Layer 3 — arld/ui/        Qt canvas widget, toolbar, panels, undo/redo stack.
                           Depends on core/ and export/.

Layer 4 — arld/app/       Entry point, session management, auto-save scheduler.
                           Depends on all lower layers.
```

Supporting directories:
- `arld/data/library/` — one JSON file per aircraft entry + `library_manifest.json`
- `arld/schemas/` — JSON Schema files (`arld-project.schema.json`, `clearance-ruleset.schema.json`)
- `arld/tests/` — Catch2 unit/integration tests + performance benchmarks

## ⚡ Key Design Constraints

- **No GPL in binaries.** All dependencies must be MIT, BSL, or LGPL (dynamically linked). Verified by `scripts/check-licenses.py` in CI.
- **`core/` has zero Qt dependency.** It must build and pass tests in a headless environment.
- **All clearance distances live in `arld/core/include/arld/core/Config.h`.** No magic numbers in `.cpp` files.
- **Coordinates stored in feet only.** Metric values are derived at render time (`value × 0.3048`); never stored in project files.
- **Undo/redo via Command pattern.** Every undoable action implements `arld::core::ICommand` (`execute()`, `undo()`, `describe()`). Commands in `arld/ui/` may use Qt types; the interface itself lives in `arld/core/` with `std::string describe()`.
- **Export via Strategy pattern.** `IExporter::export(const Layout&, const ExportOptions&)` is the sole entry point; format-specific classes implement it.
- **All inter-layer interfaces are abstract C++ classes** (pure virtual) in the lower layer's public include directory.

## 💻 Build and Test Commands

```bash
# Install C++ dependencies via vcpkg (CGAL, nlohmann-json, Catch2)
vcpkg install

# Install Qt 6.7 separately (see CONTRIBUTING.md for platform-specific instructions)
# macOS (Homebrew): brew install qt qtsvg
# Set Qt6_DIR for CMake to find Qt: export Qt6_DIR=/opt/homebrew/lib/cmake/Qt6

# Configure + build
cmake --preset linux-debug      # or mac-debug / win-debug
cmake --build --preset linux-debug

# macOS local build (if cmake --preset mac-debug fails to find Qt):
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_MAKE_PROGRAM=/opt/homebrew/bin/ninja \
  -DQt6_DIR=/opt/homebrew/lib/cmake/Qt6 \
  -DQt6Svg_DIR=/opt/homebrew/lib/cmake/Qt6Svg \
  -DQt6SvgWidgets_DIR=/opt/homebrew/lib/cmake/Qt6SvgWidgets \
  -DQt6Widgets_DIR=/opt/homebrew/lib/cmake/Qt6Widgets \
  -DQt6Core_DIR=/opt/homebrew/lib/cmake/Qt6Core \
  -B build/mac-debug .

# Run all tests
ctest --preset linux-debug --output-on-failure

# Run a single test by name pattern
ctest --preset linux-debug -R "UndoStack"
ctest --preset linux-debug -R "AircraftLibraryParser"

# Run license checker
python3 scripts/check-licenses.py

# Regenerate SVG silhouettes (after changing aircraft parameters)
python3 scripts/generate-silhouettes.py
```

CMakePresets.json defines six presets: `{mac,linux,win}-{debug,release}`.

## 🖥️ Canvas Architecture (as built)

```
RampScene   : QGraphicsScene
  ├── RampBoundaryItem    : QGraphicsPathItem    ← Sprint 0-2 ✅
  │     └── VertexHandle  : QGraphicsEllipseItem  (child, draggable)
  ├── ScaleBarItem        : QGraphicsItem         ← Sprint 0-2 ✅ (ItemIgnoresTransformations)
  ├── AircraftItem        : QGraphicsItemGroup    ← Sprint 0-3 ✅
  │     ├── ClearanceZoneItem : QGraphicsPolygonItem  ← Sprint 0-4 ✅ (zValue=-0.5; green/yellow/red)
  │     ├── QGraphicsSvgItem (silhouette, scaled to wingspan in scene-ft)
  │     └── RotationHandle  : QGraphicsEllipseItem (ItemIgnoresTransformations; visible when selected)
  ├── GridOverlayItem     : QGraphicsItem         ← Sprint 0-2+ (pending)
  └── AnnotationItem      : QGraphicsTextItem     ← Phase 1 (pending)

RampView    : QGraphicsView
  └── pan (left/middle click-drag), zoom (scroll wheel, +/- keys, AnchorUnderMouse)
```

**Coordinate system:** 1 scene unit = 1 ft. `pixelsPerFt = 12 × logicalDPI / scaleDenominator`. Default scale 1:1200; range 1:200–1:5000.

**Clearance zone colors** (Sprint 0-4): clear = `#22AA44`, advisory = `#DDAA00`, violation = `#CC2222` (fill-opacity 0.25 advisory / 0.40 violation).

## ↩️ Undo/Redo Framework (as built)

- `arld/core/include/arld/core/ICommand.h` — pure interface (`execute`, `undo`, `describe`)
- `arld/core/include/arld/core/UndoStack.h` — 100-level stack, `onChanged` callback, zero Qt dependency
- Commands in `arld/ui/` anonymous namespaces: `BoundaryAddPointCommand`, `MoveVertexCommand`, `MoveAircraftCommand`, `RotateAircraftCommand`, `PlaceCmd`
- Wired to `Ctrl+Z` / `Ctrl+Y` (all platforms) via `QKeySequence::Undo` / `QKeySequence::Redo`
- Commands already applied visually (drag-end) use a local `AlreadyExecutedWrapper` struct that skips the first `execute()` call

## ✈️ Aircraft Library (as built — Sprint 0-3)

- **Data:** `arld/data/library/` — 20 JSON entries + `library_manifest.json` + `silhouettes/` (20 SVGs)
- **Schema:** `arld/schemas/aircraft-library-entry.schema.json` (Draft-07 JSON Schema)
- **SVG silhouettes:** generated by `scripts/generate-silhouettes.py` — top-down schematics with viewBox in feet
- **Core:** `arld/core/include/arld/core/AircraftLibraryEntry.h` — struct with `AircraftCategory` and `DisplayType` enums
- **Parser:** `arld/core/src/AircraftLibraryParser.cpp` — `parseEntry(jsonStr)` and `parseManifest(jsonStr)` using nlohmann/json; zero Qt dependency
- **Qt Resources:** `arld/data/library/aircraft_library.qrc` embedded in `arld_ui` via AUTORCC; accessed at `:/library/{id}.json` and `:/library/silhouettes/{id}.svg`
- **UI:** `arld/ui/src/LibraryPanel.cpp` — `QDockWidget` with `DraggableListWidget`; initiates `application/x-arld-aircraft-id` drags
- **Drop handling:** `RampView::dropEvent` emits `aircraftDropped(id, scenePos)` → `MainWindow` → `RampScene::placeAircraft(entry, scenePos)` — placement is undoable
- **AircraftItem:** `arld/ui/src/AircraftItem.cpp` — `QGraphicsItemGroup` containing a `QGraphicsSvgItem` scaled so 1 SVG unit = 1 ft; `RotationHandle` sub-item snaps to 45° (or 1° with Shift)
- **AUTOMOC:** All Q_OBJECT headers in `arld_ui` are listed explicitly in `target_sources` (not just `.cpp` files) so CMake AUTOMOC finds them

## 🧪 Test Suite (as built)

| File | Tests | Coverage |
|------|-------|---------|
| `arld/tests/test_smoke.cpp` | 3 | Layout construction, Config constants |
| `arld/tests/test_undo.cpp` | 7 | Full UndoStack behaviour incl. 100-level depth |
| `arld/tests/test_aircraft_library.cpp` | 7 | AircraftLibraryParser — valid entries, optional fields, helicopters, manifest, error cases |
| `arld/tests/test_clearance.cpp` | 8 + 1 bench | ClearanceEngine — all 8 scenarios; bench_clearance_200 benchmark |
| `arld/tests/test_project_file.cpp` | 9 | ProjectFile round-trip (15 aircraft), UUID v4 format, schema_version rejection, invalid JSON, boundary, SVG non-empty/content/empty-validity |
| **Total** | **34 + 1 bench** | |

Run performance benchmarks with `-R bench_` (tagged `[.bench]` so excluded from the default run):
```bash
ctest --preset mac-debug -R bench_
# or directly: ./build/mac-debug/arld/tests/arld_tests "[.bench]"
```

## 📐 CGAL Geometry (as built — Sprint 0-4)

Type aliases in `arld/core/include/arld/core/GeomTypes.h`:

```cpp
using Kernel   = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point2   = Kernel::Point_2;
using Polygon2 = CGAL::Polygon_2<Kernel>;
using PolySet  = CGAL::Polygon_with_holes_2<Kernel>;
using FT       = Kernel::FT;
```

**ClearanceEngine** (`arld/core/src/ClearanceEngine.cpp`):
- `AircraftState` struct: id, wingspan/length, centre (scene-ft), rotationDeg, displayType, propArcFt
- `ViolationResult` struct: idA, idB, severity, separationFt, requiredFt
- `requiredClearanceFt(DisplayType, propArcFt)` — maps display type to hull-to-hull gap requirement
- `aircraftFootprint(AircraftState)` → rotated rectangle `Polygon2`
- `clearanceEnvelope(AircraftState)` → footprint expanded by the required clearance margin
- `detectViolations(vector<AircraftState>)` — O(N²) pairwise; uses `CGAL::squared_distance` on edge pairs; vertices checked via `bounded_side` for overlap detection
- Severity: **Violation** = below required gap; **Advisory** = within 20% above required; **Clear** = omitted from results
- `RampScene::recomputeClearance()` is debounced 80 ms after last `QGraphicsScene::changed` signal; emits `violationCountChanged(int)` to the status bar

## 💾 Project File Format (as built — Sprint 0-5)

Files use the `.arld` extension — UTF-8 JSON, schema_version 1, schema at `arld/schemas/arld-project.schema.json`. Top-level keys: `arld_version`, `schema_version`, `metadata`, `ramp_boundary`, `aircraft`. Placement IDs are UUID v4.

**`ProjectFile`** (`arld/core/src/ProjectFile.cpp`): pure C++, zero Qt dependency.
- `save(path, ProjectData)` — serializes to JSON via nlohmann/json
- `load(path)` — deserializes; throws `std::runtime_error` on wrong `schema_version` or bad JSON
- `generateUuid()` — RFC 4122 v4 UUID via `std::mt19937` seeded from `std::random_device`
- `currentUtcTimestamp()` — ISO 8601 UTC string via `gmtime_r`

**`SvgExporter`** (`arld/export/src/SvgExporter.cpp`): pure C++, no Qt; implements `IExporter`.
- Computes bbox from boundary + aircraft; 100 ft padding; max 2000 px output
- Aircraft rendered as colored, labeled, rotated rectangles; boundary as an SVG `<polygon>`
- Display-type color map (static_display `#4477AA`, warbird_heritage `#447744`, etc.)
- Namespace: `arld::export_` (trailing underscore — `export` is a reserved C++ keyword)

**Persistence flow in `RampScene`:**
- `toProjectData()` — serializes visible aircraft + boundary to `ProjectData`
- `loadProjectData(data, lookup)` — restores scene without adding undo entries
- `clearScene()` — removes all aircraft, resets boundary, clears undo stack

**`MainWindow` file operations:** New / Open / Save / Save As / Export SVG via `QFileDialog`; dirty-state tracking sets `m_dirty=true` on `QGraphicsScene::changed`; window title shows `*` suffix when dirty.

## ⚡ Performance Targets

| Scenario | Target |
|----------|--------|
| Canvas render at 200+ aircraft | 60 fps on Intel i5-8250U / integrated GPU |
| Full scene clearance re-evaluation (N=200) | < 50 ms |
| Aircraft library load (150+ entries) | < 500 ms |
| First-time user: 30-aircraft layout | Completion ≤ 60 min |

## ✅ Definition of Done (per sprint story)

1. Code merged via reviewed PR with no unresolved comments
2. All new code covered by Catch2 tests; `core/` coverage ≥ 80%
3. CI passes on all 3 platforms: build, CTest, license-check
4. Any new user-visible string wrapped in `tr()` and added to the `.ts` translation file
5. ADR updated if the story introduces a new design pattern or library dependency
