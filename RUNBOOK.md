<div align="center">
  <img src="../../CompanyTrademarks/Branding/GreenDotAirboss1Line.svg" alt="OpsNormal Airboss" width="320"/>
</div>

---

# RUNBOOK.md

Operational procedures for building, testing, and releasing the Airshow Ramp Layout Designer (ARLD).

> Updated after each sprint. **Current status:** Phase 2, Sprint 2-1 complete. 150-aircraft library (75 new entries across all categories), BatchExporter (SVG+PDF+PNG+JPEG in one call), SVG named Inkscape layers, PNG scale bar overlay, library sort combo (Name/Wingspan/Length), AircraftManifestExporter CSV, rubber-band multi-select with GroupMoveCommand undo, community submission button, export performance benchmarks. 84/84 tests pass (+ 4 benchmarks tagged [.bench]).

---

## 🔧 Build System

ARLD uses CMake 3.28 with vcpkg manifest mode. `CMakePresets.json` defines six presets:

| Preset | Platform | Config |
|--------|----------|--------|
| `mac-debug` | macOS (Apple Silicon / Intel) | Debug |
| `mac-release` | macOS | Release |
| `linux-debug` | Ubuntu 22.04 | Debug |
| `linux-release` | Ubuntu 22.04 | Release |
| `win-debug` | Windows 10/11 (MSVC 2022) | Debug |
| `win-release` | Windows 10/11 | Release |

### First-Time Setup — macOS

```bash
# 1. Install Homebrew dependencies
brew install cmake ninja qt@6 cgal

# 2. Install vcpkg and bootstrap (if not already installed)
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

# 3. Set VCPKG_ROOT (add to ~/.zshrc for persistence)
export VCPKG_ROOT=/path/to/vcpkg

# 4. Install remaining C++ dependencies via vcpkg (nlohmann-json, Catch2)
vcpkg install

# 5. Configure and build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_MAKE_PROGRAM=/opt/homebrew/bin/ninja \
  -DQt6_DIR=/opt/homebrew/lib/cmake/Qt6 \
  -DQt6Svg_DIR=/opt/homebrew/lib/cmake/Qt6Svg \
  -DQt6SvgWidgets_DIR=/opt/homebrew/lib/cmake/Qt6SvgWidgets \
  -DQt6Widgets_DIR=/opt/homebrew/lib/cmake/Qt6Widgets \
  -DQt6Core_DIR=/opt/homebrew/lib/cmake/Qt6Core \
  -B build/mac-debug .
cmake --build build/mac-debug

# 6. Run the application
./build/mac-debug/arld/app/arld
```

### First-Time Setup — Linux (Ubuntu 22.04)

```bash
# 1. Install system dependencies
sudo apt-get update
sudo apt-get install -y ninja-build gcc-13 g++-13 libgl1-mesa-dev libglu1-mesa-dev

# 2. Install vcpkg and bootstrap
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=/path/to/vcpkg

# 3. Qt 6.7 is installed by CI via jurplel/install-qt-action; for local dev use aqtinstall:
#    pip install aqtinstall && aqt install-qt linux desktop 6.7.3

# 4. Install remaining C++ dependencies via vcpkg
vcpkg install

# 5. Configure and build
cmake --preset linux-debug
cmake --build --preset linux-debug

# 6. Run the application
./build/linux-debug/arld/app/arld
```

### First-Time Setup — Windows

```bash
# 1. Install Visual Studio 2022 with C++ workload, CMake, and Ninja
# 2. Install vcpkg: git clone https://github.com/microsoft/vcpkg.git && bootstrap-vcpkg.bat
# 3. Install Qt 6.7 via Qt Installer or aqtinstall
# 4. vcpkg install
# 5. cmake --preset win-debug && cmake --build --preset win-debug
```

### Clean Build

```bash
# macOS
rm -rf build/mac-debug && cmake -G Ninja ... -B build/mac-debug . && cmake --build build/mac-debug

# Linux / Windows
rm -rf build/
cmake --preset linux-debug   # or win-debug
cmake --build --preset linux-debug
```

---

## 🧪 Running Tests

```bash
# Run all tests
ctest --preset linux-debug --output-on-failure

# Run tests matching a name pattern
ctest --preset linux-debug -R "UndoStack"
ctest --preset linux-debug -R "smoke"

# Run with verbose output
ctest --preset linux-debug -V

# Performance benchmarks (added Sprint 0-4)
ctest --preset linux-debug -R bench_
```

Coverage target: ≥ 80% on `arld/core/` — enforced in CI.

### Current Test Suite

| File | Tests | What's covered |
|------|-------|----------------|
| `arld/tests/test_smoke.cpp` | 3 | Layout construction; Config constant values and ranges |
| `arld/tests/test_undo.cpp` | 7 | UndoStack push/undo/redo, 100-level limit, redo clearing, callbacks |
| `arld/tests/test_aircraft_library.cpp` | 7 | AircraftLibraryParser — valid entries, optional fields, helicopter rotor, manifest ordering, error handling |
| `arld/tests/test_clearance.cpp` | 8 + 1 bench | ClearanceEngine — 8 scenarios (separation, warbird, military, overlap, advisory, rotation, constants) + bench_clearance_200 |
| `arld/tests/test_project_file.cpp` | 9 | ProjectFile round-trip (15 aircraft), UUID v4 format, schema_version rejection, invalid JSON, boundary, SVG non-empty/content/empty-validity |
| `arld/tests/test_unit_converter.cpp` | 5 | UnitConverter — default system, toDisplay in both units, toFeet round-trip, suffix strings |
| `arld/tests/test_schema_migration.cpp` | 6 | v1→v2 migration, bad schema version rejection, overrides round-trip, per-aircraft metadata |
| `arld/tests/test_svg_sanitizer.cpp` | 7 | SvgSanitizer — strips `<script>`, `<foreignObject>`, XXE entities, `on*` attrs, `javascript:` hrefs; clean SVG passes unchanged |
| `arld/tests/test_tail_swing.cpp` | 6 | tailSwingPolygon — empty for no-radius entry; 16-vertex poly for PT-17; center near tail; radius matches; gear-extended envelope > gear-retracted; rear extension ≥ minTurnRadiusFt |
| `arld/tests/test_pdf_exporter.cpp` | 10 | PdfExporter creates file; file non-empty; starts with %PDF; paper sizes correct; CMYK conversion; ViolationReportExporter CSV header + rows |
| `arld/tests/test_png_exporter.cpp` | 9 | PngExporter creates/non-empty/PNG magic/higher-DPI-larger; JpegExporter creates/non-empty/JPEG magic/quality-compression/dimension-cap |
| `arld/tests/test_batch_exporter.cpp` | 6 + 3 bench | BatchExporter creates 4 files/correct extensions; AircraftManifestExporter CSV header/rows/empty-project; SvgExporter inkscape:label layers; bench_export_svg/png/jpeg |
| **Total** | **84 + 4 bench** | |

---

## ⚙️ CI Pipeline

GitHub Actions runs on every push and pull request. The matrix covers all three platforms simultaneously:

| Runner | Compiler | Qt source |
|--------|----------|-----------|
| `macos-14` | Apple Clang 16 | `jurplel/install-qt-action@v4` |
| `ubuntu-22.04` | GCC 13 | `jurplel/install-qt-action@v4` |
| `windows-2022` | MSVC 2022 | `jurplel/install-qt-action@v4` |

CI jobs (in order):

| # | Job | Status |
|---|-----|--------|
| 1 | **build** — `cmake --preset release && cmake --build` | 🟢 Active |
| 2 | **test** — `ctest --preset release --output-on-failure` | 🟢 Active |
| 3 | **license-check** — `python3 scripts/check-licenses.py` | 🟢 Active |
| 4 | **lint** — clang-tidy static analysis (continue-on-error baseline) | 🟡 Active (Sprint 1-1) |
| 5 | **schema-validate** — JSON schema validation | ⬜ Planned (Phase 1) |

A PR cannot merge unless build, test, and license-check pass on all three platforms.

---

## 🔒 License Compliance

No GPL-licensed code may appear in distributable binaries. Permitted licenses: MIT, BSL-1.0, Apache 2.0, LGPL (dynamically linked only).

```bash
# Run license checker locally
python3 scripts/check-licenses.py

# Review the generated inventory
cat LICENSES.txt
```

`scripts/check-licenses.py` maintains a `DEPENDENCY_LICENSES` dict of all known direct and transitive dependencies. When adding a new dependency:
1. Add it to the dict with its SPDX identifier.
2. Verify no `GPL-` or `AGPL-` prefix appears.
3. Run the checker locally before pushing.

---

## ✈️ Development Workflow

### Canvas Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `B` | Toggle boundary draw mode |
| `+` / `=` | Zoom in (anchored to viewport centre) |
| `-` | Zoom out |
| Scroll wheel | Zoom in/out (anchored to cursor) |
| Left-click drag (empty area) | Pan |
| Middle-click drag | Pan |
| `Ctrl+Z` / `Cmd+Z` | Undo |
| `Ctrl+Y` / `Cmd+Shift+Z` | Redo |
| `Ctrl+0` / `Cmd+0` | Fit to Window (frames ramp with 50 ft margin) |
| `G` | Toggle grid overlay |
| `M` | Toggle metric/imperial display |
| Shift (while drawing/dragging) | Disable snap-to-grid |
| Left-click drag (empty area) | Rubber-band lasso multi-select |
| Shift+click aircraft | Add/remove from selection |
| Drag selected aircraft | Move entire selection group (one undoable command) |
| Escape | Clear current selection |

### Boundary Drawing Workflow

1. Press `B` (or use the Draw menu) to enter boundary-draw mode.
2. Click to add each vertex — points snap to 5 ft grid by default.
3. Double-click to close the polygon (requires ≥ 3 points).
4. Drag any vertex handle to reshape; each move creates an undoable command.

### Clearance Zone Colours

| Status | Colour | Meaning |
|--------|--------|---------|
| Clear | `#22AA44` (green) | ≥ 120% of required gap |
| Advisory | `#DDAA00` (amber) | 100–120% of required gap |
| Violation | `#CC2222` (red) | Below required gap or overlapping |

The status bar shows a live violation count. The clearance re-evaluation runs at most every 80 ms (debounced via `QTimer`). Advisory threshold is 20 % above the required minimum.

### Batch Export Workflow

**File → Export All Formats...** calls `BatchExporter::exportAll(data, basePath, options)` in a single step. It writes four files with the same base name: `.svg`, `.pdf`, `.png`, and `.jpg`. Any per-format failure (e.g., libharu unavailable) is caught internally and reported as a summary error message after all formats are attempted.

```cpp
// Example: export to /tmp/show_layout.{svg,pdf,png,jpg}
arld::export_::ExportOptions opts;
opts.paperSize    = arld::export_::PaperSize::ANSI_D;
opts.orientation  = arld::export_::Orientation::Landscape;
opts.showScaleBar = true;
arld::export_::BatchExporter::exportAll(data, "/tmp/show_layout", opts);
```

**File → Export Aircraft Manifest CSV...** calls `AircraftManifestExporter::exportCsv(data, path)`. The CSV contains one row per placed aircraft with columns: Placement ID, Library ID, Display Name, Tail Number, Owner, Fuel Type, Hazmat, Display Type, Center X (ft), Center Y (ft), Heading (deg).

### Multi-Select and Group Move

- **Rubber-band lasso:** left-click drag on empty canvas area draws a selection rectangle; all aircraft whose shape intersects the rubber band are selected.
- **Shift-click:** add/remove individual aircraft from the current selection.
- **Group move:** drag any selected aircraft to move the entire selection together. The move is pushed onto the undo stack as a single `GroupMoveCommand` covering all moved items.
- **Escape:** clears the current selection.

### Adding New Undoable Actions

1. Define a command class in the relevant `.cpp` file (anonymous namespace):
   ```cpp
   class MyActionCommand : public arld::core::ICommand {
       void execute() override { /* apply */ }
       void undo()    override { /* revert */ }
       std::string describe() const override { return "My Action"; }
   };
   ```
2. Push it: `m_undoStack.push(std::make_unique<MyActionCommand>(...));`
3. If the action is already applied visually before pushing (e.g., drag-end), wrap it in `AlreadyExecutedWrapper` (see `RampScene.cpp`) so `push()` doesn't re-execute.

---

## 🗺️ Aircraft Library

The aircraft library lives in `arld/data/library/` — one JSON file per aircraft entry. Entry IDs follow the format `{manufacturer_code}-{model_code}-{variant_code}` (e.g., `north-american-p51-d`). All 20 Phase 0 aircraft are implemented.

### 150 Aircraft Library (Sprint 2-1)

The library now contains 150 entries spanning all categories: WWII warbirds (prop fighters and bombers), Korean/Vietnam-era jets, modern jet fighters (4th and 5th gen), strategic bombers, heavy transports, business jets, helicopters, aerobatic, and general aviation. All entries follow the same JSON schema and carry dimensional data verified against ≥ 3 published sources.

Run `python3 scripts/generate-silhouettes.py` to regenerate all 150 SVG silhouettes from the parametric generator.

### 20 Phase 0 Aircraft (Sprint 0-3)

| ID | Name | Category |
|----|------|----------|
| `north-american-p51-d` | P-51D Mustang | Warbird |
| `supermarine-spitfire-ixc` | Spitfire Mk.IXc | Warbird |
| `grumman-f6f-5` | F6F-5 Hellcat | Warbird |
| `curtiss-p40-n` | P-40N Warhawk | Warbird |
| `republic-p47-d` | P-47D Thunderbolt | Warbird |
| `boeing-b17-g` | B-17G Flying Fortress | Bomber |
| `north-american-b25-j` | B-25J Mitchell | Bomber |
| `douglas-a26-c` | A-26C Invader | Bomber |
| `mcdonnell-douglas-f4-e` | F-4E Phantom II | Jet Fighter |
| `general-dynamics-f16-a` | F-16A Fighting Falcon | Jet Fighter |
| `mcdonnell-douglas-fa18-c` | F/A-18C Hornet | Jet Fighter |
| `lockheed-martin-f22-a` | F-22A Raptor | Jet Fighter |
| `lockheed-martin-f35-a` | F-35A Lightning II | Jet Fighter |
| `northrop-grumman-b2-a` | B-2A Spirit | Bomber |
| `lockheed-c130-h` | C-130H Hercules | Heavy Transport |
| `boeing-c17-a` | C-17A Globemaster III | Heavy Transport |
| `boeing-b52-h` | B-52H Stratofortress | Bomber |
| `extra-ea300-l` | Extra EA-300L | Aerobatic |
| `pitts-s2-c` | Pitts S-2C | Aerobatic |
| `bell-oh58-d` | OH-58D Kiowa Warrior | Helicopter |

### Adding a New Aircraft Entry

1. Create a new JSON file in `arld/data/library/` conforming to `arld/schemas/aircraft-library-entry.schema.json`.
2. Run `python3 scripts/generate-silhouettes.py` to generate the SVG, or create an original SVG in `arld/data/library/silhouettes/`.
3. Add the entry ID to `library_manifest.json` (controls display order).
4. Add the new JSON and SVG files to `arld/data/library/aircraft_library.qrc`.
5. Cross-reference dimensional data against at least three published sources; note them in the `data_sources` field.

IDs are permanent — never reassign or reuse a retired entry ID.

---

## 💾 Project File Format (as built — Sprint 0-5)

Project files use the `.arld` extension (UTF-8 JSON, schema_version 1). Schema at `arld/schemas/arld-project.schema.json`. `ProjectFile::save()` and `::load()` live in `arld/core/src/ProjectFile.cpp` (pure C++, zero Qt dependency). `::load()` throws `std::runtime_error` on a wrong `schema_version` or malformed JSON.

### File Menu Operations

| Action | Key | Behaviour |
|--------|-----|-----------|
| New | `Ctrl+N` | Prompts if dirty; clears scene and undo stack |
| Open | `Ctrl+O` | Prompts if dirty; opens `.arld` file via `QFileDialog` |
| Save | `Ctrl+S` | Saves to current path; calls Save As if no current path |
| Save As | `Ctrl+Shift+S` | Prompts for path then saves |
| Export SVG | — | Exports scaled SVG via `SvgExporter`; does not change current path |

### Schema Migration

When `schema_version` is incremented:
1. Write a migration function in `arld/core/ProjectMigration.cpp`.
2. The application auto-migrates on file open and prompts the user to save in the new version.
3. Update `arld/schemas/arld-project.schema.json`.

---

## 📦 Packaging and Release (Phase 1+)

```bash
# macOS — CPack .dmg
cmake --build --preset mac-release --target package

# Linux — CPack .AppImage + .deb
cmake --build --preset linux-release --target package

# Windows — CPack .msi (requires WiX Toolset)
cmake --build --preset win-release --target package
```

---

## 📋 Sprint Definitions

### Phase 1 — Production Desktop (Months 3–6)

#### Sprint 1-1 · Month 3, Wk 1–2 · 41 pts
**Goal:** clang-tidy CI, CPack packaging, UnitConverter, auto-save, exporter stubs, 50-aircraft library

| # | Story | Pts |
|---|-------|-----|
| 1-1-1 | clang-tidy CI step (continue-on-error baseline) | 3 |
| 1-1-2 | CPack platform-gated generators (.dmg / .AppImage+.deb / .msi) | 5 |
| 1-1-3 | UnitConverter singleton (ft↔m, suffix strings) | 3 |
| 1-1-4 | Auto-save: 60 s QTimer → CacheLocation/arld_autosave.arld; crash-recovery dialog at startup | 8 |
| 1-1-5 | SvgExporter fully implemented (replaces stub from Sprint 0-5) | 5 |
| 1-1-6 | IExporter interface + PdfExporter / PngExporter / JpegExporter stubs | 3 |
| 1-1-7 | Expand aircraft library to 50 entries; CI schema validation step | 8 |
| 1-1-8 | JSON Schema validation CI step for .arld project files | 3 |
| 1-1-9 | UnitConverter Catch2 tests (5 tests) | 3 |

#### Sprint 1-2 · Month 3, Wk 3–4 · 48 pts
**Goal:** ViolationsPanel, ClearanceRuleSet (FAA CoW + ICAS), PropertiesPanel, clearance overrides, hazmat, schema v1→v2 migration

| # | Story | Pts |
|---|-------|-----|
| 1-2-1 | ViolationsPanel dock: live list of active violations, sortable by severity | 8 |
| 1-2-2 | ClearanceRuleSet struct: FAA CoW + ICAS factory methods; pass to ClearanceEngine | 5 |
| 1-2-3 | Rule Set selector in toolbar; settings persisted in .arld file | 3 |
| 1-2-4 | PropertiesPanel dock: tail number, owner, fuel type, hazmat checkbox, notes | 8 |
| 1-2-5 | Per-aircraft hazmat flag: red ⚠ overlay icon on canvas; persists in .arld | 5 |
| 1-2-6 | Clearance override dialog: justification text, username, timestamp; orange override zone colour | 8 |
| 1-2-7 | Schema v1→v2 migration: auto-migrate on load; prompt to save in new version | 5 |
| 1-2-8 | Catch2 tests for schema migration (6 tests: v1→v2 round-trip, bad version rejection, overrides, per-aircraft metadata) | 3 |
| 1-2-9 | ClearanceSeverity::Overridden rendered orange (#DD7700) in zone and violations panel | 3 |

#### Sprint 1-3 · Month 4, Wk 1–2 · 47 pts
**Goal:** Library browser UI; custom aircraft entries; heading controls; 75-aircraft library

| # | Story | Pts |
|---|-------|-----|
| 1-3-1 | Library browser panel: text search filtered by manufacturer/model; category filter dropdown | 8 |
| 1-3-2 | Thumbnail silhouette preview (48×48), wingspan, and length in each library browser row | 5 |
| 1-3-3 | Custom aircraft entry dialog: all required dimensional fields; validate ranges; save to user library | 8 |
| 1-3-4 | SVG sanitization for user-uploaded silhouettes: strip XXE, `<script>`, `<foreignObject>` | 5 |
| 1-3-5 | Exact heading input field (0–359°) on properties panel; snap-to-runway-heading control | 5 |
| 1-3-6 | Expand aircraft library to 75 entries; all entries pass CI schema validation | 8 |
| 1-3-7 | Coordinate grid overlay (GridOverlayItem); configurable spacing (25/50/100 ft); togglable | 5 |
| 1-3-8 | Fit-to-Window and recently-opened projects list (last 10) | 3 |

#### Sprint 1-4 · Month 4, Wk 3–4 · 44 pts
**Goal:** Tail-dragger tail-swing, extended/retracted gear, accessibility foundation

| # | Story | Pts |
|---|-------|-----|
| 1-4-1 | Tail-dragger tail-swing arc: swept polygon from `min_turn_radius_ft`; added to clearance envelope | 8 |
| 1-4-2 | Extended/retracted gear clearance variants; gear state selector on properties panel | 5 |
| 1-4-3 | Taxi-Only corridor (50 ft lane) and Military Static standoff (50 ft perimeter) zone renderers | 5 |
| 1-4-4 | Media/Photo Platform crush-barrier overlay (QGraphicsPolygonItem, configurable shape) | 3 |
| 1-4-5 | Ramp Show Aircraft 200 ft crowd-line clearance zone | 3 |
| 1-4-6 | QAccessible on AircraftItem and RampBoundaryItem; accessibleName = tail number + display type | 5 |
| 1-4-7 | Wrap all user-visible strings in `tr()`; configure lupdate CI step; create en.ts baseline | 5 |
| 1-4-8 | Keyboard navigation: all toolbar controls, panels, dropdowns accessible via Tab | 5 |
| 1-4-9 | Accessibility audit (axe or Qt Accessibility Inspector); document WCAG 2.1 AA baseline | 5 |

#### Sprint 1-5 · Month 5, Wk 1–2 · 45 pts
**Goal:** PDF export (libharu), satellite underlay, violation export report

| # | Story | Pts |
|---|-------|-----|
| 1-5-1 | PdfExporter (libharu 2.4): Letter, Tabloid, ANSI-C/D/E/E1 paper sizes | 13 |
| 1-5-2 | Embed Inter font subset (SIL OFL) via `HPDF_LoadTTFontFromFile`; title block at bottom 1.5 in | 8 |
| 1-5-3 | ICC-aware CMYK colour conversion; `/OutputIntents` in PDF (PDF/X-1a compliance) | 5 |
| 1-5-4 | QR code in PDF title block (SHA-256 of .arld file) using libqrencode (LGPL) | 3 |
| 1-5-5 | Display type legend in PDF footer; portrait/landscape orientation selector | 3 |
| 1-5-6 | Satellite underlay: async image load on worker thread; opacity slider 0–100% | 8 |
| 1-5-7 | Export Violations Report: PDF/CSV listing all violations + overrides with justification text | 5 |

#### Sprint 1-6 · Month 6, Wk 1–2 · 43 pts
**Goal:** PNG/JPEG export, project save polish, alpha release to 10 beta users

| # | Story | Pts |
|---|-------|-----|
| 1-6-1 | PngExporter (stb_image_write): DPI presets {72,96,150,300,600}; transparent + white background | 5 |
| 1-6-2 | JpegExporter (stb_image_write): quality 60–100% in 5% steps; max 32767 px with warning | 5 |
| 1-6-3 | Golden reference files in `arld/tests/golden/` via Git LFS; CI export pHash tests | 5 |
| 1-6-4 | .arld file versioning: schema_version=1 (Phase 1 canonical); migration from v0 PoC files | 3 |
| 1-6-5 | Recently-used files list; project metadata dialog (show name, date, venue, units) | 3 |
| 1-6-6 | Alpha installer distribution (.dmg, .AppImage, .msi) to 10 beta users; structured feedback | 5 |
| 1-6-7 | Triage alpha feedback; create bug backlog; update risk register | 3 |
| 1-6-8 | Code coverage report: core/ and export/ ≥ 80%; Codecov CI gate active | 5 |
| 1-6-9 | Locale-aware decimal parsing: QLocale::system() for input; always serialise as `.` in JSON | 3 |
| 1-6-10 | NPS survey setup for beta cohort | 2 |
| 1-6-11 | Update LICENSES.txt; run license compliance check | 4 |

---

### Phase 2 — Feature Complete (Months 7–12)

#### Sprint 2-1 · Month 7, Wk 1–2 · 50 pts ✅ Complete
**Goal:** 150-aircraft library; batch export; SVG named layers

| # | Story | Pts |
|---|-------|-----|
| 2-1-1 | Expand aircraft library to 150 entries; dimensional data verified ≥ 3 sources | 13 |
| 2-1-2 | BatchExporter: all 4 formats in one action using last-saved settings | 5 |
| 2-1-3 | Named SVG layers (`<g id='...'>` with inkscape:label); layer visibility at export | 5 |
| 2-1-4 | Scale bar overlay in PNG export; export configuration save/load | 3 |
| 2-1-5 | Library browser: sort by name, wingspan, length; placement count badge | 3 |
| 2-1-6 | CSV metadata manifest export alongside diagram export | 3 |
| 2-1-7 | Multi-select and group move (Shift+click or drag lasso) | 5 |
| 2-1-8 | Community entry submission portal (dialog → GitHub issue template in browser) | 3 |
| 2-1-9 | Performance regression: bench_canvas and bench_geom pass budgets with 150-type library | 5 |
| 2-1-10 | Export performance tests: SVG ≤ 3 s, PDF ≤ 8 s, PNG ≤ 6 s, JPEG ≤ 5 s (200-aircraft ANSI-D) | 5 |

#### Sprint 2-2 · Month 7, Wk 3–4 · 44 pts
**Goal:** Layout versioning, change-delta view, minimap, geo-referenced tile underlay

| # | Story | Pts |
|---|-------|-----|
| 2-2-1 | Named layout snapshots (versions[]): save, list, switch within one .arld file | 8 |
| 2-2-2 | Visual change-delta view: added (green), removed (red), moved (amber) aircraft between versions | 8 |
| 2-2-3 | Version export as standalone .arld file | 2 |
| 2-2-4 | Minimap overview panel with viewport rectangle indicator | 5 |
| 2-2-5 | Opt-in geo-referenced tile streaming underlay (Mapbox/Google Static Maps); opacity slider | 8 |
| 2-2-6 | Undo/redo history panel (last 20 actions); accessible via View menu | 5 |
| 2-2-7 | KML/GeoJSON boundary import; convert to logical-ft polygon | 5 |
| 2-2-8 | Tag-triggered CPack packaging; attach to GitHub Release; publish community edition | 3 |

#### Sprint 2-3 · Month 8–9, Wk 1–2 · 47 pts
**Goal:** Opt-in library update service; LOD rendering; beta release

| # | Story | Pts |
|---|-------|-----|
| 2-3-1 | Opt-in library update check: HTTPS fetch of library_manifest.json; SHA-256 comparison; incremental download | 8 |
| 2-3-2 | LOD rendering: at zoom < 1:2000 render AircraftItems as bounding rectangles | 5 |
| 2-3-3 | Arrival/departure time fields; fuel type auto-trigger no-smoking overlay in Hot Ramp mode | 3 |
| 2-3-4 | Aircraft label toggle (tail number / call sign) per aircraft; persists in .arld | 2 |
| 2-3-5 | Locale-based decimal separator in all numeric inputs; lupdate CI green | 3 |
| 2-3-6 | WCAG contrast audit CI tooling; pattern fills as supplement to colour coding for CVD users | 5 |
| 2-3-7 | Performance benchmark suite: add TRD-PERF-007–010 timed tests; trend chart in CI | 5 |
| 2-3-8 | Beta release to 25 users via GitHub Releases; NPS survey link in release notes | 3 |
| 2-3-9 | Open-source license compliance review; update LICENSES.txt and NOTICES.txt | 5 |
| 2-3-10 | Security review: nlohmann/json SAX parser depth/length limits; network code compile-time flags | 5 |
| 2-3-11 | Catch2 test: .arld file nesting depth > 32 rejected with error | 3 |

#### Sprint 2-4 · Month 9–10, Wk 3–4 · 44 pts
**Goal:** UAT event 1; bug fixes; snap-to-runway and group move polish

| # | Story | Pts |
|---|-------|-----|
| 2-4-1 | UAT session at Airshow Event 1 (30-aircraft scenario); screen-capture all sessions | 8 |
| 2-4-2 | Triage UAT Event 1 bugs; fix all P1 issues before next UAT | 8 |
| 2-4-3 | Polish snap-to-runway: multi-select + snap aligns all selected aircraft to primary heading | 3 |
| 2-4-4 | Polish group move: lasso selection, arrow-key nudge (1 ft / 5 ft with Shift) | 3 |
| 2-4-5 | Export Violations Report PDF: structured table with gap, minimum, severity, override justification | 5 |
| 2-4-6 | Add placement count and total area statistics to status bar | 3 |
| 2-4-7 | Performance regression: all 10 TRD-PERF budgets pass after UAT fixes | 5 |
| 2-4-8 | Accessibility regression: keyboard navigation and VoiceOver pass after UAT fixes | 5 |
| 2-4-9 | Minimap and LOD rendering regression tests | 4 |

#### Sprint 2-5 · Month 10–11, Wk 1–2 · 46 pts
**Goal:** UAT events 2 & 3; export quality review with print vendors

| # | Story | Pts |
|---|-------|-----|
| 2-5-1 | UAT session at Airshow Event 2 (30-aircraft scenario) | 5 |
| 2-5-2 | UAT session at Airshow Event 3 (30-aircraft scenario) | 5 |
| 2-5-3 | Aggregate UAT task completion stats across 3 events; confirm ≥ 85% pass rate | 3 |
| 2-5-4 | Submit PDF layouts to 3 commercial print vendors; evaluate proofs | 5 |
| 2-5-5 | Fix print-vendor issues (ICC profile, bleed, colour shift); re-submit for approval | 5 |
| 2-5-6 | Triage UAT Events 2–3 bugs; fix all P1 and P2 issues | 8 |
| 2-5-7 | NPS survey on 25-user beta cohort; collect qualitative feedback | 2 |
| 2-5-8 | Performance benchmark full suite on all 3 platforms; commit results as release artifacts | 5 |
| 2-5-9 | Scale bar variants: imperial only, metric only, dual; user selects per export | 3 |
| 2-5-10 | Update NOTICES.txt; prepare draft LICENSES.txt for Phase 3 open-source audit | 5 |

#### Sprint 2-6 · Month 11–12, Wk 3–4 · 44 pts
**Goal:** Security review; open-source license audit; Phase 2 hardening

| # | Story | Pts |
|---|-------|-----|
| 2-6-1 | Formal open-source license audit (Legal + Lead Eng); confirm Apache 2.0 community edition compliance | 8 |
| 2-6-2 | Security review: file-parsing limits, SVG sanitization, HTTPS enforcement, no telemetry without consent | 8 |
| 2-6-3 | Full performance benchmark suite; Phase 2 benchmark report PDF | 5 |
| 2-6-4 | Resolve remaining P2 bugs from UAT; full regression suite on all 3 platforms | 8 |
| 2-6-5 | Phase 2 release notes; update README with build instructions and feature summary | 3 |
| 2-6-6 | Confirm NPS ≥ 40; or file remediation plan with steering committee | 2 |
| 2-6-7 | Prepare Phase 3 gate materials: UAT sign-off, benchmark report, security report, license audit, NPS | 5 |
| 2-6-8 | Fix outstanding WCAG 2.1 AA accessibility issues; accessibility audit sign-off | 5 |

---

### Phase 3 — Desktop Launch (Months 13–14)

#### Sprint 3-1 · Month 13, Wk 1–2 · 40 pts
**Goal:** Public v1.0 release on all platforms; GitHub community edition launch

| # | Story | Pts |
|---|-------|-----|
| 3-1-1 | Signed production release builds triggered by v1.0.0 tag; all 4 packages attached to GitHub Release | 8 |
| 3-1-2 | Publish community edition source (public GitHub repo, Apache 2.0 LICENSE, CONTRIBUTING.md, CLA bot) | 5 |
| 3-1-3 | Project website / landing page (GitHub Pages): features, screenshots, download links, docs | 5 |
| 3-1-4 | ARLD listing submitted to ICAS marketplace / resources page | 3 |
| 3-1-5 | Adoption dashboard: GitHub Release download counts + opt-in telemetry event counter | 5 |
| 3-1-6 | ARLD v1.0 documentation site: user guide, aircraft library schema, .arld file format spec, FAQ | 5 |
| 3-1-7 | Monitor GitHub Issues for v1.0 launch bugs; hotfix any P1 within 48 hours | 5 |
| 3-1-8 | Steering committee presentation: Phase 3 launch metrics, NPS, adoption counts, Phase 4 readiness | 4 |

#### Sprint 3-2 · Month 13–14, Wk 3–4 · 38 pts
**Goal:** Commercial license tier; ICAS marketplace; adoption monitoring

| # | Story | Pts |
|---|-------|-----|
| 3-2-1 | Commercial license tier: key validation, feature unlock (priority support, volume exports) | 8 |
| 3-2-2 | Commercial license distribution: Gumroad/Stripe payment link; key delivery by email | 5 |
| 3-2-3 | Monitor adoption KPI: target 25 active events within 6 months; weekly reporting | 3 |
| 3-2-4 | Phase 4 business case preparation materials: adoption metrics, NPS, cost estimate, steering committee deck | 5 |
| 3-2-5 | Address top 5 community edition feature requests from GitHub Issues; v1.1.0 patch | 8 |
| 3-2-6 | Phase 3 retrospective; update risk register; close Phase 0–3 budget actuals | 3 |
| 3-2-7 | Begin Phase 4 team recruitment (web, cloud, TypeScript/React) pending approval | 3 |
| 3-2-8 | 6-month post-launch KPI checkpoint: confirm adoption ≥ 25 events, NPS ≥ 40 for Phase 4 gate | 3 |

---

### Phase 4 — SaaS Platform (Pending Steering Committee Approval)

> **Notice:** Phase 4 cannot begin until the Phase 3 gate criteria are met and the steering committee approves the Phase 4 business case with dedicated funding. Sprint dates are relative to the Phase 4 kickoff date (month +0).

#### Sprint 4-1 · Month +1 to +2 · 48 pts
**Goal:** Architecture ADR; WebAssembly PoC vs REST benchmark; React canvas prototype

| # | Story | Pts |
|---|-------|-----|
| 4-1-1 | Architecture ADR: benchmark WebAssembly (Emscripten) vs REST microservice on 100-aircraft edit workload; select strategy | 13 |
| 4-1-2 | WebAssembly PoC or REST microservice prototype (per ADR outcome) | 13 |
| 4-1-3 | React canvas prototype: place, move, rotate aircraft using geometry from PoC | 8 |
| 4-1-4 | Auth0 integration prototype: login, JWT, user session | 8 |
| 4-1-5 | AWS infrastructure skeleton (ECS Fargate, RDS, S3); IaC in CDK or Terraform | 6 |

---

## 🚦 Phase Gate Checklists

### Phase 0 → Phase 1 Gate (PoC Sign-Off)

- [x] GitHub Actions CI passes clean builds on all 3 matrix targets
- [x] 20 aircraft types in library with correct scaled silhouette rendering *(Sprint 0-3 ✅)*
- [x] Test layout: 15 aircraft created, saved, reloaded, SVG-exported with no data loss *(Sprint 0-5 ✅)*
- [x] Clearance violations correctly detected for ≥ 5 scenarios in the test suite *(Sprint 0-4 ✅)*
- [ ] Domain expert completes a 10-aircraft layout in < 20 minutes unassisted *(Sprint 0-5)*
- [x] No GPL-licensed code in deliverable binaries (license-check CI step passes)

### Phase 1 → Phase 2 Gate (Production Desktop)

- [ ] 75+ aircraft types in library; all entries pass CI schema validation *(Sprint 1-3)*
- [ ] PDF, PNG, JPEG exporters functional; golden-file CI test passing *(Sprint 1-5/1-6)*
- [ ] Alpha installer distributed to 10 beta users; structured feedback collected *(Sprint 1-6)*
- [ ] Code coverage: core/ and export/ ≥ 80% on all 3 platforms *(Sprint 1-6)*
- [ ] All CI jobs passing: build, test, license-check, lint, schema-validate

### Phase 2 → Phase 3 Gate (Beta Sign-Off)

- [ ] 150+ aircraft types in library; all entries pass CI schema validation *(Sprint 2-1)*
- [ ] All export formats (SVG, PDF, PNG, JPEG, Batch) functional; golden-file CI passing *(Sprint 2-1)*
- [ ] UAT task completion rate ≥ 85% across 3 real airshow events *(Sprint 2-5)*
- [ ] PDF print quality: 3 vendors confirm ≥ 4.0/5.0 satisfaction *(Sprint 2-5)*
- [ ] All 10 TRD-PERF performance budgets met on all 3 platforms *(Sprint 2-6)*
- [ ] NPS ≥ 40 from beta user cohort *(Sprint 2-5)*
- [ ] Security review and open-source license audit reports filed and signed off *(Sprint 2-6)*
- [ ] Zero P1/P2 open bugs; WCAG 2.1 AA compliance verified *(Sprint 2-6)*

### Phase 3 → Phase 4 Gate (SaaS Initiation)

- [ ] v1.0.0 production release published on all 3 platforms *(Sprint 3-1)*
- [ ] Apache 2.0 community edition source public on GitHub; CLA bot active *(Sprint 3-1)*
- [ ] Commercial license tier live; payment processing functional *(Sprint 3-2)*
- [ ] ICAS marketplace listing confirmed *(Sprint 3-1)*
- [ ] ≥ 50 active airshow events using ARLD v1.0 for primary ramp planning (6-month window) *(Sprint 3-2)*
- [ ] Net Promoter Score (NPS) ≥ 40 from show directors (6 months post-launch) *(Sprint 3-2)*
- [ ] Written Phase 4 business case approved by steering committee with dedicated funding *(Sprint 3-2)*
- [ ] Phase 4 team recruitment underway or complete *(Sprint 3-2)*

---

## 📅 Sprint Cadence

- Sprint duration: 2 weeks
- Story sizing: Fibonacci (1, 2, 3, 5, 8, 13)
- Velocity targets: 28–32 pts/sprint (Phase 0) → 42–50 pts/sprint (Phases 1–2)

**Definition of Done** for each story:
1. Code merged via reviewed PR with no unresolved comments
2. Catch2 tests added; `core/` coverage remains ≥ 80%
3. CI passes build, test, and license-check on all 3 platforms
4. Any new user-visible string wrapped in `tr()` and added to the `.ts` translation file
5. ADR updated if story introduces a new design pattern or library

---

<div align="center">
  <img src="../../CompanyTrademarks/Branding/GreenDot.svg" alt="OpsNormal Airboss" width="36"/>
  <br/>
  <sub><em>Start Small. Stay Safe. Fly the Show.</em></sub>
</div>
