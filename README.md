<div align="center">
  <img src="https://raw.githubusercontent.com/OpsNormal-Airboss/CompanyTrademarks/main/Branding/GreenDotAirboss2Line.svg" alt="OpsNormal Airboss" width="360"/>
  <br/>
  <em>Start Small. Stay Safe. Fly the Show.</em>
  <br/><br/>

  [![CI](https://github.com/OpsNormal-Airboss/RampDesigner/actions/workflows/ci.yml/badge.svg)](https://github.com/OpsNormal-Airboss/RampDesigner/actions/workflows/ci.yml)
  [![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-00CC00.svg)](./LICENSE)
  [![Phase 0 — PoC](https://img.shields.io/badge/phase-0%20%E2%80%94%20PoC%20Sprint%203%2F5-00CC00)](./RUNBOOK.md)
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
| **0** | **C++ PoC** — 20 aircraft, clearance engine, SVG export, JSON save/load | **🟢 In Progress — Sprint 2 of 5** |
| 1 | Production desktop, 75+ aircraft, all display types, satellite underlay | ⬜ Not started |
| 2 | 150+ aircraft, full export suite (SVG/PDF/PNG/JPEG), UAT | ⬜ Not started |
| 3 | Public v1.0 desktop release, open-source community edition | ⬜ Not started |
| 4 | SaaS platform (cloud-hosted) — pending steering committee approval | ⬜ Not started |

### Phase 0 Sprint Progress

| Sprint | Goal | Status |
|--------|------|--------|
| 0-1 | CMake/vcpkg scaffold, CI matrix, `Config.h`, smoke tests | ✅ Complete |
| 0-2 | Qt canvas — pan/zoom, boundary drawing, undo/redo framework | ✅ Complete |
| 0-3 | 20-aircraft library, SVG silhouettes, drag-and-drop placement | ✅ Complete |
| 0-4 | CGAL clearance zones, real-time violation detection | ⬜ Up next |
| 0-5 | SVG export, JSON project save/load, PoC acceptance gate | ⬜ Pending |

## 🟢 What Works Today

The application compiles and runs on macOS, Linux, and Windows. The CI pipeline is green on all three platforms.

- **Qt window** with menu bar, toolbar (undo/redo), and status bar showing current scale
- **Interactive canvas** — pan (left/middle click-drag), zoom (scroll wheel, `+`/`-` keys), scale range 1:200–1:5000
- **Ramp boundary drawing** — press `B` to enter draw mode, click to add vertices, double-click to close
- **Vertex editing** — drag any vertex handle to reshape the boundary in real time
- **Snap-to-grid** — 5 ft grid by default; hold Shift to draw freehand
- **Scale bar** — live overlay showing correct footage at any zoom level
- **Undo/redo** — 100-level history; `Ctrl+Z` / `Ctrl+Y` (or `Cmd+Z` / `Cmd+Shift+Z` on macOS)
- **Aircraft library panel** — 20 aircraft across all major categories (WWII warbirds, jet fighters, heavy transports, bombers, aerobatic) with accurate dimensional data
- **Drag-and-drop placement** — drag from library panel onto canvas; silhouettes render at true geographic scale (1 scene unit = 1 ft)
- **Aircraft rotation** — click to select, drag the rotation handle to rotate; snaps to 45° (or hold Shift for 1° precision); fully undoable

## ✈️ Features (Phase 0–3 Desktop, full scope)

- Interactive 2D canvas (Qt 6) with pan, zoom, snap-to-grid, and 100-level undo/redo
- Aircraft library: 20 types (PoC) → 150+ types (Phase 2) with accurate dimensional data and SVG silhouettes
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

Current test suite: 10 Catch2 tests across `test_smoke.cpp` and `test_undo.cpp`.

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
  <img src="https://raw.githubusercontent.com/OpsNormal-Airboss/CompanyTrademarks/main/Branding/GreenDot.svg" alt="OpsNormal Airboss" width="40"/>
  <br/>
  <sub>An <a href="https://github.com/OpsNormal-Airboss">OpsNormal Airboss</a> project</sub>
</div>
