<div align="center">
  <img src="../../CompanyTrademarks/Branding/GreenDotAirboss2Line.svg" alt="OpsNormal Airboss" width="360"/>
  <br/>
  <em>Start Small. Stay Safe. Fly the Show.</em>
  <br/><br/>

  [![CI](https://github.com/OpsNormal-Airboss/RampDesigner/actions/workflows/ci.yml/badge.svg)](https://github.com/OpsNormal-Airboss/RampDesigner/actions/workflows/ci.yml)
  [![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-00CC00.svg)](./LICENSE)
  [![Phase 2 — Sprint 1/6](https://img.shields.io/badge/phase-2%20%E2%80%94%20Sprint%201%2F6-00CC00)](./RUNBOOK.md)
  [![C++20](https://img.shields.io/badge/C%2B%2B-20-1A1610.svg)](https://isocpp.org/)
  [![Qt 6.7](https://img.shields.io/badge/Qt-6.7%20LGPL-1A1610.svg)](https://www.qt.io/)
</div>

---

# RampDesigner — Airshow Ramp Layout Designer (ARLD)

A safety-critical C++ desktop application for designing, validating, and publishing aircraft parking layouts for static and flying airshows. ARLD enforces FAA Certificate of Waiver (CoW) clearance rules in real time and produces print-quality export diagrams in SVG, PDF, PNG, and JPEG.

## ✈️ Why ARLD

Airshow ramp planning is a safety-critical activity. Incorrectly spaced aircraft create FOD hazards, impede emergency egress routes, and violate FAA CoW separation requirements. Current workflows rely on hand-drawn sketches, generic CAD tools, or spreadsheets — none of which provide real-time scaling, aircraft-type awareness, or standards-compliance checking.

ARLD provides a purpose-built visual design environment where every aircraft silhouette, parking node, and service lane is rendered at true geographic scale.

## 🗺️ Project Status

| Phase | Description | Status |
|-------|-------------|--------|
| **0** | **C++ PoC** — 20 aircraft, clearance engine, SVG export, JSON save/load | **✅ Complete** |
| **1** | **Production desktop, 75+ aircraft, all display types, satellite underlay** | **✅ Complete — All 6 sprints done** |
| **2** | **150+ aircraft, full export suite, multi-select, batch export, UAT** | **🔄 In Progress — Sprint 1/6** |
| 3 | Public v1.0 desktop release, open-source community edition | ⬜ Not started |
| 4 | SaaS platform (cloud-hosted) — pending steering committee approval | ⬜ Not started |

### Phase 0 Sprint Progress — ✅ Complete

| Sprint | Goal | Status |
|--------|------|--------|
| 0-1 | CMake/vcpkg scaffold, CI matrix, `Config.h`, smoke tests | ✅ Complete |
| 0-2 | Qt canvas — pan/zoom, boundary drawing, undo/redo framework | ✅ Complete |
| 0-3 | 20-aircraft library, SVG silhouettes, drag-and-drop placement | ✅ Complete |
| 0-4 | CGAL clearance zones, real-time violation detection | ✅ Complete |
| 0-5 | SVG export, JSON project save/load, PoC acceptance gate | ✅ Complete |

### Phase 1 Sprint Progress — ✅ Complete

| Sprint | Goal | Status |
|--------|------|--------|
| 1-1 | PoC-to-production refactor; CPack installers; UnitConverter; auto-save; exporter stubs; 50-aircraft library | ✅ Complete |
| 1-2 | Violations panel; clearance rule config; display type assignment UI; overrides; hazmat | ✅ Complete |
| 1-3 | Library browser; custom aircraft; heading controls; 75-aircraft library | ✅ Complete |
| 1-4 | Tail-dragger tail-swing; extended gear; corridor/standoff zones; accessibility | ✅ Complete |
| 1-5 | PDF export (libharu); satellite underlay; violations report | ✅ Complete |
| 1-6 | PNG/JPEG export (stb_image_write); vendored header; raster tests | ✅ Complete |

### Phase 2 Sprint Progress

| Sprint | Goal | Status |
|--------|------|--------|
| 2-1 | 150-aircraft library; batch export; SVG named layers; multi-select; AircraftManifestExporter CSV | ✅ Complete |
| 2-2 | Layout versioning; change-delta view; minimap; geo-referenced tile underlay | ⬜ Up next |
| 2-3 | Opt-in library update service; LOD rendering; beta release | ⬜ Not started |
| 2-4 | UAT event 1; bug fixes; snap-to-runway and group move polish | ⬜ Not started |
| 2-5 | UAT events 2 & 3; export quality review with print vendors | ⬜ Not started |
| 2-6 | Security review; open-source license audit; Phase 2 hardening | ⬜ Not started |

## 🟢 What Works Today

The application compiles and runs on macOS, Linux, and Windows. The CI pipeline is green on all three platforms.

- **Qt window** with menu bar, toolbar (undo/redo), and status bar showing current scale
- **Interactive canvas** — pan (left/middle click-drag), zoom (scroll wheel, `+`/`-` keys), scale range 1:200–1:5000
- **Ramp boundary drawing** — press `B` to enter draw mode, click to add vertices, double-click to close
- **Vertex editing** — drag any vertex handle to reshape the boundary in real time
- **Snap-to-grid** — 5 ft grid by default; hold Shift to draw freehand
- **Scale bar** — live overlay showing correct footage at any zoom level
- **Undo/redo** — 100-level history; `Ctrl+Z` / `Ctrl+Y` (or `Cmd+Z` / `Cmd+Shift+Z` on macOS)
- **Aircraft library browser** — 150 aircraft with text search, category filter, sort by Name/Wingspan/Length, and 48×48 SVG thumbnail previews; wingspan and length shown in current unit system; community submission button opens GitHub issue template in browser
- **Custom aircraft entry** — Add Custom Aircraft... dialog with full dimensional form, SVG sanitization (strips XSS/XXE), and save to local user library
- **Drag-and-drop placement** — drag from library panel onto canvas; silhouettes render at true geographic scale (1 scene unit = 1 ft)
- **Aircraft rotation + heading input** — drag the rotation handle (snaps 45° / 1° with Shift) or type an exact 0–359° heading; "Snap All Selected to Heading" button aligns multi-aircraft groups; fully undoable
- **Grid overlay** — configurable 25/50/100 ft spacing; toggle with `G`; recomputes on zoom
- **Tail-swing arcs** — tail-dragger aircraft (PT-17, P-51, Spitfire, etc.) display an amber swept-arc polygon showing the tail-swing radius; factored into clearance envelopes
- **Extended/retracted gear** — aircraft with retractable gear show a gear state toggle (Gear Extended / Retracted) that expands or contracts the clearance zone by 8 ft
- **Display-type zone rendering** — Taxi-Only shows a narrow corridor shape; Military Static uses a thick red dashed border; Ramp Show uses a thick amber DashDot boundary
- **CVD accessibility** — clearance zone fills use hatch patterns (diagonal/cross-hatch/horizontal lines) in addition to color, supporting color-vision-deficient users
- **PDF export** — File → Export PDF exports to Letter, Tabloid, ANSI C/D/E/E1 in portrait or landscape; title block with show name, date, venue, version, export date, and QR code (SHA-256); display type legend; optional violations report page
- **PNG export** — File → Export PNG produces a raster diagram at any DPI preset (72/96/150/300/600); pure-C++ scanline rasterizer; white background; max 16384 px per side
- **JPEG export** — File → Export JPEG produces a compressed raster at quality 1–100; max 32767 px per side (warns to stderr if capped); no additional dependencies beyond vendored stb_image_write.h
- **Batch export** — File → Export All Formats exports SVG, PDF, PNG, and JPEG in one action using last-saved settings; any per-format failure is reported without aborting the remaining formats
- **Aircraft manifest CSV** — File → Export Aircraft Manifest CSV produces a spreadsheet of all placed aircraft with placement ID, tail number, owner, fuel type, hazmat flag, display type, and position
- **Multi-select and group move** — rubber-band lasso (drag on empty canvas) selects multiple aircraft; moving any selected aircraft moves the whole group; fully undoable as a single GroupMoveCommand; Escape clears selection
- **SVG named layers** — exported SVGs include Inkscape-compatible `<g id=... inkscape:label=...>` layers for Ramp Boundary, Aircraft, and Annotations; layer-aware editing in Inkscape/Illustrator works out of the box
- **Violations report** — File → Export Violations Report exports a PDF table or CSV with all violation pairs, measured/required gaps, severity, and override justifications
- **Satellite underlay** — View → Load Satellite Image imports a JPEG/PNG; loads asynchronously without blocking the UI; opacity slider 0–100%
- **Real-time clearance zones** — each aircraft displays a coloured envelope (green = clear, yellow = advisory, red = violation); recomputed within 80 ms of any change
- **Violation status bar** — shows live count of clearance violations across all placed aircraft
- **FAA CoW clearance rules** — per-display-type separation requirements enforced: Static Display (25 ft), Warbird/Heritage (prop arc + 35 ft), Military Static (50 ft), Hot Ramp (100 ft), Ramp Show (200 ft), Media Platform (15 ft)
- **Project save/load** — File → Save / Open persist the full layout (aircraft placement, rotation, boundary) to `.arld` JSON files; File → New prompts to discard unsaved changes
- **SVG export** — File → Export SVG produces a scaled diagram with the ramp boundary, all aircraft (colored by display type), and labels
- **Metric/Imperial toggle** — View → Show in Metric switches all distance displays between feet and metres without data loss
- **Auto-save** — layout is auto-saved every 60 seconds; crash recovery dialog offered on next launch if previous session ended unexpectedly
- **Violations panel** — bottom dock lists all active violations with severity, measured gap, and required gap; click any row to center the view on the offending aircraft pair
- **Clearance rule configuration** — Tools → Clearance Rules... lets show directors switch between FAA CoW and ICAS Standard templates or set custom per-type distances; custom rules persist in the `.arld` file
- **Properties panel** — right dock shows display type (all 7 types), tail number, owner, fuel type, and hazmat flag for any selected aircraft; changing display type immediately recalculates clearance zones
- **Clearance overrides** — violations can be overridden with a justification (min 20 characters); overridden pairs render in orange and are stored in the project file with username and timestamp
- **Hazmat indicator** — aircraft marked as carrying hazardous materials show a ⚠ warning icon on the canvas

## ✈️ Features (Phase 0–3 Desktop, full scope)

- Interactive 2D canvas (Qt 6) with pan, zoom, snap-to-grid, and 100-level undo/redo
- Aircraft library: 20 types (PoC) → 150 types (Phase 2, Sprint 2-1) with accurate dimensional data and SVG silhouettes; sort by name, wingspan, or length
- Seven display types with type-specific FAA clearance rules: Static Display, Warbird/Heritage, Taxi-Only, Military Static, Hot Ramp, Media/Photo Platform, Ramp Show
- Real-time clearance violation detection and green/yellow/red zone overlays
- Satellite/aerial photograph underlay with opacity control
- Export: SVG, PDF (Letter–ANSI-E1), PNG (72–600 DPI), JPEG
- Offline operation — no network required for core functionality
- Runs natively on macOS 12+, Ubuntu 22.04 LTS, and Windows 10/11

## 🔧 Getting Started

### Prerequisites

- **CMake** 3.28+
- **vcpkg** — set `VCPKG_ROOT` to your vcpkg installation directory
- **Qt 6.7** (LGPL) — install separately; see platform notes in [CONTRIBUTING.md](./CONTRIBUTING.md)
- **Ninja** build system
- A C++20 compiler: Apple Clang 16+ (macOS), GCC 13+ (Linux), or MSVC 2022 (Windows)

### Build

```bash
# Install C++ dependencies (CGAL, nlohmann-json, Catch2)
vcpkg install

# Configure
cmake --preset linux-debug      # or mac-debug / win-debug

# Build
cmake --build --preset linux-debug

# Run
./build/linux-debug/arld
```

### Test

```bash
ctest --preset linux-debug --output-on-failure
```

Current test suite: 84 Catch2 tests (+ 4 benchmarks) across `test_smoke.cpp`, `test_undo.cpp`, `test_aircraft_library.cpp`, `test_clearance.cpp`, `test_project_file.cpp`, `test_unit_converter.cpp`, `test_schema_migration.cpp`, `test_svg_sanitizer.cpp`, `test_tail_swing.cpp`, `test_pdf_exporter.cpp`, `test_png_exporter.cpp`, and `test_batch_exporter.cpp`.

## 📋 Documentation

Planning documents are in [`docs/`](./docs/):

| Document | Description |
|----------|-------------|
| `ARLD_Project_Charterdocx.docx` | Project charter, phases, budget, risk register |
| `ARLD_Functional_Requirements_v1.0.docx` | Full functional requirements (MoSCoW prioritized) |
| `ARLD_Technical_Requirements_v1.0.docx` | Engineering constraints, data schemas, build pipeline |
| `ARLD_Project_Plan_v1.0.docx` | Sprint-by-sprint delivery schedule, all phases |

See [`RUNBOOK.md`](./RUNBOOK.md) for build, test, and operational procedures.

## 🏗️ Architecture

The desktop application is a C++20 Qt 6 application structured into four strictly layered modules:

```
arld/core/      Geometry engine, clearance rules, aircraft library, project I/O (no Qt)
arld/export/    SVG, PDF, PNG, JPEG exporters (strategy pattern)
arld/ui/        Qt canvas, toolbar, panels, undo/redo stack
arld/app/       Application entry point and session management
```

All clearance distances (FAA CoW defaults) are defined in `arld/core/include/arld/core/Config.h`. Undo/redo uses the Command pattern via `arld::core::ICommand`. Project files use the open `.arld` JSON format with a published schema (Sprint 0-5).

## 🔒 License

Community edition: Apache 2.0. All dependencies are MIT, BSL, or LGPL (dynamically linked) — no GPL code in distributable binaries.

---

<div align="center">
  <img src="../../CompanyTrademarks/Branding/GreenDot.svg" alt="OpsNormal Airboss" width="40"/>
  <br/>
  <sub>An <a href="https://github.com/OpsNormal-Airboss">OpsNormal Airboss</a> project</sub>
</div>
