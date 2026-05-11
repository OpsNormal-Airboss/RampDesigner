<div align="center">
  <img src="https://raw.githubusercontent.com/OpsNormal-Airboss/CompanyTrademarks/main/Branding/GreenDotAirboss1Line.svg" alt="OpsNormal Airboss" width="320"/>
</div>

---

# RUNBOOK.md

Operational procedures for building, testing, and releasing the Airshow Ramp Layout Designer (ARLD).

> Updated after each sprint. **Current status:** Phase 0 complete — all 5 sprints done. Build system, CI, Qt canvas, undo/redo, 20-aircraft library, CGAL clearance engine, SVG export, and JSON project save/load are all operational. 34/34 tests pass. Phase 0 PoC acceptance gate passed.

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

### First-Time Setup

```bash
# 1. Install vcpkg and bootstrap (if not already installed)
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh       # macOS/Linux
# or bootstrap-vcpkg.bat on Windows

# 2. Set VCPKG_ROOT (add to shell profile for persistence)
export VCPKG_ROOT=/path/to/vcpkg

# 3. Install Qt 6.7 (see CONTRIBUTING.md for platform-specific instructions)
#    Qt is NOT managed by vcpkg — install separately via aqtinstall or system package manager

# 4. Install remaining C++ dependencies via vcpkg (CGAL, nlohmann-json, Catch2)
vcpkg install

# 5. Configure and build
cmake --preset linux-debug
cmake --build --preset linux-debug

# 6. Run the application
./build/linux-debug/arld
```

### Clean Build

```bash
rm -rf build/
cmake --preset linux-debug
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
| **Total** | **34 + 1 bench** | |

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
| 4 | **lint** — clang-tidy static analysis | ⬜ Planned (Phase 1) |
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
| Shift (while drawing/dragging) | Disable snap-to-grid |

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

## 🚦 Phase Gate Checklists

### Phase 0 → Phase 1 Gate (PoC Sign-Off)

- [x] GitHub Actions CI passes clean builds on all 3 matrix targets
- [x] 20 aircraft types in library with correct scaled silhouette rendering *(Sprint 0-3 ✅)*
- [x] Test layout: 15 aircraft created, saved, reloaded, SVG-exported with no data loss *(Sprint 0-5 ✅)*
- [x] Clearance violations correctly detected for ≥ 5 scenarios in the test suite *(Sprint 0-4 ✅)*
- [ ] Domain expert completes a 10-aircraft layout in < 20 minutes unassisted *(Sprint 0-5)*
- [x] No GPL-licensed code in deliverable binaries (license-check CI step passes)

### Phase 3 → Phase 4 Gate (SaaS Initiation)

- [ ] ≥ 50 active airshow events using ARLD v1.0 for primary ramp planning
- [ ] Net Promoter Score (NPS) ≥ 40 from show directors (6 months post-launch)
- [ ] Written Phase 4 business case approved by steering committee with dedicated funding

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
  <img src="https://raw.githubusercontent.com/OpsNormal-Airboss/CompanyTrademarks/main/Branding/GreenDot.svg" alt="OpsNormal Airboss" width="36"/>
  <br/>
  <sub><em>Start Small. Stay Safe. Fly the Show.</em></sub>
</div>
