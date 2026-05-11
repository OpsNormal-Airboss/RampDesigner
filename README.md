# RampDesigner — Airshow Ramp Layout Designer (ARLD)

A safety-critical C++ desktop application for designing, validating, and publishing aircraft parking layouts for static and flying airshows. ARLD enforces FAA Certificate of Waiver (CoW) clearance rules in real time and produces print-quality export diagrams in SVG, PDF, PNG, and JPEG.

## Why ARLD

Airshow ramp planning is a safety-critical activity. Incorrectly spaced aircraft create FOD hazards, impede emergency egress routes, and violate FAA CoW separation requirements. Current workflows rely on hand-drawn sketches, generic CAD tools, or spreadsheets — none of which provide real-time scaling, aircraft-type awareness, or standards-compliance checking.

ARLD provides a purpose-built visual design environment where every aircraft silhouette, parking node, and service lane is rendered at true geographic scale.

## Project Status

| Phase | Description | Status |
|-------|-------------|--------|
| 0 | C++ PoC — 20 aircraft, clearance engine, SVG export, JSON save/load | Planning |
| 1 | Production desktop, 75+ aircraft, all display types, satellite underlay | Not started |
| 2 | 150+ aircraft, full export suite (SVG/PDF/PNG/JPEG), UAT | Not started |
| 3 | Public v1.0 desktop release, open-source community edition | Not started |
| 4 | SaaS platform (cloud-hosted) — pending steering committee approval | Not started |

## Features (Phase 0–3 Desktop)

- Interactive 2D canvas (Qt 6) with pan, zoom, snap-to-grid, and 100-level undo/redo
- Aircraft library: 20 types (PoC) → 150+ types (Phase 2) with accurate dimensional data and SVG silhouettes
- Seven display types with type-specific FAA clearance rules: Static Display, Warbird/Heritage, Taxi-Only, Military Static, Hot Ramp, Media/Photo Platform, Ramp Show
- Real-time clearance violation detection and green/yellow/red zone overlays
- Satellite/aerial photograph underlay with opacity control
- Export: SVG, PDF (Letter–ANSI-E1), PNG (72–600 DPI), JPEG
- Offline operation — no network required for core functionality
- Runs natively on macOS 12+, Ubuntu 22.04 LTS, and Windows 10/11

## Getting Started

> Setup instructions will be added after Sprint 0-1 establishes the CMake/vcpkg baseline.

### Prerequisites

- CMake 3.28+
- vcpkg
- Qt 6.7 (LGPL)
- A C++20 compiler: Apple Clang 16+ (macOS), GCC 13+ (Linux), or MSVC 2022 (Windows)

### Build

```bash
vcpkg install
cmake --preset <platform>-release    # e.g. mac-release, linux-release, win-release
cmake --build --preset <platform>-release
```

### Test

```bash
ctest --preset <platform>-debug
```

## Documentation

Planning documents are in [`docs/`](./docs/):

| Document | Description |
|----------|-------------|
| `ARLD_Project_Charterdocx.docx` | Project charter, phases, budget, risk register |
| `ARLD_Functional_Requirements_v1.0.docx` | Full functional requirements (MoSCoW prioritized) |
| `ARLD_Technical_Requirements_v1.0.docx` | Engineering constraints, data schemas, build pipeline |
| `ARLD_Project_Plan_v1.0.docx` | Sprint-by-sprint delivery schedule, all phases |

See [`RUNBOOK.md`](./RUNBOOK.md) for operational procedures.

## Architecture

The desktop application is a C++20 Qt 6 application structured into four strictly layered modules:

```
arld/core/      Geometry engine, clearance rules, aircraft library, project I/O (no Qt)
arld/export/    SVG, PDF, PNG, JPEG exporters
arld/ui/        Qt canvas, toolbar, panels, undo/redo stack
arld/app/       Application entry point and session management
```

All clearance distances (FAA CoW defaults) are defined in `arld/core/Config.h`. Geometry computations use CGAL 5.6. Project files use the open `.arld` JSON format with a published schema.

## License

Community edition: Apache 2.0. All dependencies are MIT, BSL, or LGPL (dynamically linked) — no GPL code in distributable binaries.
