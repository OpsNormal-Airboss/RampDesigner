# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**Airshow Ramp Layout Designer (ARLD)** — a safety-critical C++ desktop application for designing, validating, and publishing aircraft parking layouts for static and flying airshows. It enforces FAA Certificate of Waiver (CoW) clearance rules in real time and exports print-quality diagrams.

The project is in **pre-code planning stage** (Phase 0 starts with Sprint 0-1). All planning documents are in `docs/`.

## Post-Sprint Documentation

After completing each sprint, update the following files to reflect the current state of the project:

- **CLAUDE.md** — update commands, architecture, and any guidance that has changed
- **README.md** — update the getting started guide, feature summary, and setup instructions
- **RUNBOOK.md** — update operational procedures, deployment steps, and runbook entries for any new or changed functionality (create the file if it does not exist)

## Phases

| Phase | Scope | Timeline |
|-------|-------|----------|
| 0 | C++ PoC — 20 aircraft, clearance engine, SVG export, JSON save/load | Weeks 1–10 (5 sprints) |
| 1 | Production desktop, 75 aircraft, all display types, satellite underlay | Months 3–6 |
| 2 | 150+ aircraft, full export suite, UAT | Months 7–12 |
| 3 | Public v1.0 release, open-source community edition, commercial tier | Months 13–14 |
| 4 | SaaS platform (TypeScript/React + AWS) — **pending steering committee approval** | Post Phase 3 |

## Tech Stack

**Phases 0–3 (Desktop)**
- Language: C++20 (Clang 16+, GCC 13+, MSVC 2022)
- GUI/Canvas: Qt 6.7 LGPL — `QGraphicsScene`/`QGraphicsView`
- Geometry: CGAL 5.6 LGPL — Minkowski sums, polygon intersection, spatial indexing
- JSON/Persistence: nlohmann/json 3.11
- PDF Export: libharu 2.4
- Raster Export: stb_image_write (header-only)
- Build: CMake 3.28 + vcpkg manifest mode
- Testing: Catch2 3.x + CTest
- CI: GitHub Actions (macos-14 / ubuntu-22.04 / windows-2022 matrix)
- Packaging: CPack → `.dmg` / `.AppImage`+`.deb` / `.msi`

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
                           Depends on core/ only.

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

- **No GPL in binaries.** All dependencies must be MIT, BSL, or LGPL (dynamically linked). Verified by a `license-check` CI job on every commit.
- **`core/` has zero Qt dependency.** It must build and pass tests in a headless environment.
- **All clearance distances live in `arld/core/Config.h`.** No magic numbers in `.cpp` files.
- **Coordinates stored in feet only.** Metric values are derived at render time (`value × 0.3048`); never stored in project files.
- **Undo/redo via Command pattern.** Every undoable action is a class implementing `ICommand` with `execute()`, `undo()`, and `describe()`.
- **Export via Strategy pattern.** `IExporter::export(const Layout&, const ExportOptions&)` is the sole entry point; format-specific classes implement it.
- **All inter-layer interfaces are abstract C++ classes** (pure virtual) in the lower layer's public include directory.

## Build and Test Commands

> _Commands will be confirmed and updated after Sprint 0-1 establishes the CMake/vcpkg baseline._

```bash
# Install dependencies (vcpkg manifest mode)
vcpkg install

# Configure + build (replace <preset> with e.g. mac-debug, linux-release, win-release)
cmake --preset <preset>
cmake --build --preset <preset>

# Run tests
ctest --preset <preset>

# Run a single test by name
ctest --preset linux-debug -R <test_name>

# Performance benchmarks
ctest --preset linux-debug -R bench_
```

CMakePresets.json defines six presets: `{mac,linux,win}-{debug,release}`.

## Canvas Architecture

```
RampScene   : QGraphicsScene
  ├── RampBoundaryItem    : QGraphicsPathItem
  ├── AircraftItem        : QGraphicsItemGroup
  │     ├── SilhouetteItem  : QGraphicsSvgItem
  │     └── ClearanceZoneItem : QGraphicsPathItem
  ├── GridOverlayItem     : QGraphicsItem  (custom paint)
  ├── ScaleBarItem        : QGraphicsItem  (custom paint)
  └── AnnotationItem      : QGraphicsTextItem

RampView    : QGraphicsView
  └── pan (click-drag), zoom (wheel/pinch), AnchorUnderMouse
```

Clearance zone colors: clear = `#22AA44`, advisory = `#DDAA00`, violation = `#CC2222`, override = `#E07000` (fill-opacity 0.25 advisory / 0.40 violation).

## CGAL Geometry

Required kernel and type aliases are defined in `arld/core/GeomTypes.h`:

```cpp
using Kernel   = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point2   = Kernel::Point_2;
using Polygon2 = CGAL::Polygon_2<Kernel>;
using PolySet  = CGAL::Polygon_with_holes_2<Kernel>;
using FT       = Kernel::FT;
```

Clearance envelopes use `CGAL::minkowski_sum_2()`. Violation detection uses `CGAL::do_intersect()` with gap measurement via `CGAL::squared_distance()`. Aircraft convex hulls are precomputed at library load time and cached.

## Project File Format

Files use the `.arld` extension — UTF-8 JSON with a published schema at `arld/schemas/arld-project.schema.json`. Top-level keys: `arld_version`, `schema_version`, `metadata`, `ramp_boundary`, `clearance_rules`, `aircraft`, `versions`, `overrides`. Placement IDs are UUID v4. Schema migration functions are required for each `schema_version` increment.

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
3. CI passes on all 3 platforms: build, clang-tidy lint, CTest, license-check
4. Any new user-visible string wrapped in `tr()` and added to the `.ts` translation file
5. ADR updated if the story introduces a new design pattern or library dependency
