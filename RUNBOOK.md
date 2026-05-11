<div align="center">
  <img src="https://raw.githubusercontent.com/OpsNormal-Airboss/CompanyTrademarks/main/Branding/GreenDotAirboss1Line.svg" alt="OpsNormal Airboss" width="320"/>
</div>

---

# RUNBOOK.md

Operational procedures for building, testing, and releasing the Airshow Ramp Layout Designer (ARLD).

> Updated after each sprint. **Current status:** Phase 0, Sprint 0-2 complete. Build system, CI, Qt canvas, and undo/redo framework are all operational.

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
| **Total** | **10** | |

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
| 4 | **lint** — clang-tidy static analysis | ⬜ Planned (Sprint 0-3) |
| 5 | **schema-validate** — JSON schema validation | ⬜ Planned (Sprint 0-5) |

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

The aircraft library lives in `arld/data/library/` — one JSON file per aircraft entry (Sprint 0-3). Entry IDs follow the format `{manufacturer_code}-{model_code}-{variant_code}` (e.g., `north-american-p51-d`).

### Adding a New Aircraft Entry

1. Create a new JSON file in `arld/data/library/` conforming to the `AircraftLibraryEntry` schema (see `arld/schemas/`).
2. Source or create an original SVG silhouette scaled to the published wingspan.
3. Update `library_manifest.json` with the new entry ID and its SHA-256 file hash.
4. Cross-reference dimensional data against at least three published sources; note them in the `data_source` field.

IDs are permanent — never reassign or reuse a retired entry ID.

---

## 💾 Project File Format (Sprint 0-5, pending)

Project files will use the `.arld` extension (UTF-8 JSON). Schema at `arld/schemas/arld-project.schema.json`.

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
- [ ] 20 aircraft types in library with correct scaled silhouette rendering *(Sprint 0-3)*
- [ ] Test layout: 15 aircraft created, saved, reloaded, SVG-exported with no data loss *(Sprint 0-5)*
- [ ] Clearance violations correctly detected for ≥ 5 scenarios in the test suite *(Sprint 0-4)*
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
