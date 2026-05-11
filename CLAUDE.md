# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**Airshow Ramp Layout Designer (ARLD)** — a safety-critical C++ desktop application for designing, validating, and publishing aircraft parking layouts for static and flying airshows. It enforces FAA Certificate of Waiver (CoW) clearance rules in real time and exports print-quality diagrams.

**Current status:** Phase 0 (C++ PoC) — Sprints 0-1 and 0-2 complete. The build system, CI pipeline, Qt canvas prototype, undo/redo framework, and boundary drawing are all implemented. Next up is Sprint 0-3 (20-aircraft library + drag-and-drop placement).

## Post-Sprint Documentation

After completing each sprint, update the following files to reflect the current state of the project:

- **CLAUDE.md** — update commands, architecture, and any guidance that has changed
- **README.md** — update the getting started guide, feature summary, and setup instructions
- **RUNBOOK.md** — update operational procedures, deployment steps, and runbook entries for any new or changed functionality

## Phases

| Phase | Scope | Timeline |
|-------|-------|----------|
| 0 | C++ PoC — 20 aircraft, clearance engine, SVG export, JSON save/load | Weeks 1–10 (5 sprints) |
| 1 | Production desktop, 75 aircraft, all display types, satellite underlay | Months 3–6 |
| 2 | 150+ aircraft, full export suite, UAT | Months 7–12 |
| 3 | Public v1.0 release, open-source community edition, commercial tier | Months 13–14 |
| 4 | SaaS platform (TypeScript/React + AWS) — **pending steering committee approval** | Post Phase 3 |

## Sprint Log (Phase 0)

| Sprint | Goal | Status |
|--------|------|--------|
| 0-1 | CMake/vcpkg scaffold, CI matrix, Config.h, smoke tests | ✅ Complete |
| 0-2 | Qt canvas prototype — pan/zoom, boundary drawing, undo/redo | ✅ Complete |
| 0-3 | 20-aircraft library, SVG silhouettes, drag-and-drop placement | ⬜ Not started |
| 0-4 | CGAL clearance zones, real-time violation detection | ⬜ Not started |
| 0-5 | SVG export, JSON project save/load, PoC acceptance gate | ⬜ Not started |

## Tech Stack

**Phases 0–3 (Desktop)**
- Language: C++20 (Clang 16+, GCC 13+, MSVC 2022)
- GUI/Canvas: Qt 6.7 LGPL — `QGraphicsScene`/`QGraphicsView`
- Geometry: CGAL 5.6 LGPL — Minkowski sums, polygon intersection, spatial indexing
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

## Module Architecture

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

## Key Design Constraints

- **No GPL in binaries.** All dependencies must be MIT, BSL, or LGPL (dynamically linked). Verified by `scripts/check-licenses.py` in CI.
- **`core/` has zero Qt dependency.** It must build and pass tests in a headless environment.
- **All clearance distances live in `arld/core/include/arld/core/Config.h`.** No magic numbers in `.cpp` files.
- **Coordinates stored in feet only.** Metric values are derived at render time (`value × 0.3048`); never stored in project files.
- **Undo/redo via Command pattern.** Every undoable action implements `arld::core::ICommand` (`execute()`, `undo()`, `describe()`). Commands in `arld/ui/` may use Qt types; the interface itself lives in `arld/core/` with `std::string describe()`.
- **Export via Strategy pattern.** `IExporter::export(const Layout&, const ExportOptions&)` is the sole entry point; format-specific classes implement it.
- **All inter-layer interfaces are abstract C++ classes** (pure virtual) in the lower layer's public include directory.

## Build and Test Commands

```bash
# Install C++ dependencies via vcpkg (CGAL, nlohmann-json, Catch2)
vcpkg install

# Install Qt 6.7 separately (see CONTRIBUTING.md for platform-specific instructions)

# Configure + build
cmake --preset linux-debug      # or mac-debug / win-debug
cmake --build --preset linux-debug

# Run all tests
ctest --preset linux-debug --output-on-failure

# Run a single test by name pattern
ctest --preset linux-debug -R "UndoStack"

# Run license checker
python3 scripts/check-licenses.py
```

CMakePresets.json defines six presets: `{mac,linux,win}-{debug,release}`.

## Canvas Architecture (as built)

```
RampScene   : QGraphicsScene
  ├── RampBoundaryItem    : QGraphicsPathItem    ← Sprint 0-2 ✅
  │     └── VertexHandle  : QGraphicsEllipseItem  (child, draggable)
  ├── ScaleBarItem        : QGraphicsItem         ← Sprint 0-2 ✅ (ItemIgnoresTransformations)
  ├── AircraftItem        : QGraphicsItemGroup    ← Sprint 0-3 (pending)
  │     ├── SilhouetteItem  : QGraphicsSvgItem
  │     └── ClearanceZoneItem : QGraphicsPathItem
  ├── GridOverlayItem     : QGraphicsItem         ← Sprint 0-2+ (pending)
  └── AnnotationItem      : QGraphicsTextItem     ← Phase 1 (pending)

RampView    : QGraphicsView
  └── pan (left/middle click-drag), zoom (scroll wheel, +/- keys, AnchorUnderMouse)
```

**Coordinate system:** 1 scene unit = 1 ft. `pixelsPerFt = 12 × logicalDPI / scaleDenominator`. Default scale 1:1200; range 1:200–1:5000.

**Clearance zone colors** (Sprint 0-4): clear = `#22AA44`, advisory = `#DDAA00`, violation = `#CC2222`, override = `#E07000` (fill-opacity 0.25 advisory / 0.40 violation).

## Undo/Redo Framework (as built)

- `arld/core/include/arld/core/ICommand.h` — pure interface (`execute`, `undo`, `describe`)
- `arld/core/include/arld/core/UndoStack.h` — 100-level stack, `onChanged` callback, zero Qt dependency
- Commands in `arld/ui/` anonymous namespaces: `BoundaryAddPointCommand`, `MoveVertexCommand`
- Wired to `Ctrl+Z` / `Ctrl+Y` (all platforms) via `QKeySequence::Undo` / `QKeySequence::Redo`
- Vertex-drag commands use an `AlreadyExecutedWrapper` so `push()` doesn't re-apply a change already applied visually

## Test Suite (as built)

| File | Tests | Coverage |
|------|-------|---------|
| `arld/tests/test_smoke.cpp` | 3 | Layout construction, Config constants |
| `arld/tests/test_undo.cpp` | 7 | Full UndoStack behaviour incl. 100-level depth |

## CGAL Geometry (Sprint 0-4, pending)

Required kernel and type aliases will be defined in `arld/core/GeomTypes.h`:

```cpp
using Kernel   = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point2   = Kernel::Point_2;
using Polygon2 = CGAL::Polygon_2<Kernel>;
using PolySet  = CGAL::Polygon_with_holes_2<Kernel>;
using FT       = Kernel::FT;
```

Clearance envelopes: `CGAL::minkowski_sum_2()`. Violation detection: `CGAL::do_intersect()` + `CGAL::squared_distance()`. Convex hulls precomputed at library load time.

## Project File Format (Sprint 0-5, pending)

Files use the `.arld` extension — UTF-8 JSON with a published schema at `arld/schemas/arld-project.schema.json`. Top-level keys: `arld_version`, `schema_version`, `metadata`, `ramp_boundary`, `clearance_rules`, `aircraft`, `versions`, `overrides`. Placement IDs are UUID v4.

## Performance Targets

| Scenario | Target |
|----------|--------|
| Canvas render at 200+ aircraft | 60 fps on Intel i5-8250U / integrated GPU |
| Full scene clearance re-evaluation (N=200) | < 50 ms |
| Aircraft library load (150+ entries) | < 500 ms |
| First-time user: 30-aircraft layout | Completion ≤ 60 min |

## Definition of Done (per sprint story)

1. Code merged via reviewed PR with no unresolved comments
2. All new code covered by Catch2 tests; `core/` coverage ≥ 80%
3. CI passes on all 3 platforms: build, CTest, license-check
4. Any new user-visible string wrapped in `tr()` and added to the `.ts` translation file
5. ADR updated if the story introduces a new design pattern or library dependency
