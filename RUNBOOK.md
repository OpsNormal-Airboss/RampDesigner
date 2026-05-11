# RUNBOOK.md

Operational procedures for building, testing, and releasing the Airshow Ramp Layout Designer (ARLD).

> This file is updated at the end of each sprint to reflect the current state of the project.
> **Current status:** Pre-code — commands and procedures will be populated starting Sprint 0-1.

---

## Build System

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
./vcpkg/bootstrap-vcpkg.sh   # macOS/Linux
# or bootstrap-vcpkg.bat on Windows

# 2. Install all project dependencies (reads vcpkg.json manifest)
vcpkg install

# 3. Configure and build
cmake --preset linux-debug
cmake --build --preset linux-debug
```

### Clean Build

```bash
rm -rf build/
cmake --preset linux-debug
cmake --build --preset linux-debug
```

---

## Running Tests

```bash
# Run all tests
ctest --preset linux-debug

# Run a single test by name
ctest --preset linux-debug -R <test_name>

# Run performance benchmarks
ctest --preset linux-debug -R bench_

# Run with verbose output
ctest --preset linux-debug -V
```

Coverage target: ≥ 80% on `arld/core/` — enforced in CI.

---

## CI Pipeline

GitHub Actions runs on every push and pull request. The matrix covers all three platforms simultaneously:

| Runner | Compiler | Artifact |
|--------|----------|----------|
| `macos-14` | Apple Clang 16 | `.dmg` |
| `ubuntu-22.04` | GCC 13 | `.AppImage` + `.deb` |
| `windows-2022` | MSVC 2022 | `.msi` |

CI jobs (in order):
1. **build** — `cmake --preset release && cmake --build --preset release`
2. **test** — `ctest --preset release`
3. **lint** — `clang-tidy` static analysis; no cross-layer dependency violations
4. **license-check** — SPDX scan; fails if any GPL code is present in the dependency graph
5. **schema-validate** — validates `arld/schemas/*.schema.json` against any changed data files

A PR cannot merge unless all five jobs pass on all three platforms.

---

## License Compliance

No GPL-licensed code may appear in distributable binaries. Permitted licenses: MIT, BSL-1.0, Apache 2.0, LGPL (dynamically linked only).

```bash
# Run license checker locally (requires the license-checker CI tool installed)
# Exact command TBD — populated in Sprint 0-1

# Review transitive license inventory
cat LICENSES.txt
```

If a new dependency is added, update `LICENSES.txt` and verify the license-check CI job passes before merging.

---

## Aircraft Library

The aircraft library lives in `arld/data/library/` — one JSON file per aircraft entry. Entry IDs follow the format `{manufacturer_code}-{model_code}-{variant_code}` (e.g., `north-american-p51-d`).

### Adding a New Aircraft Entry

1. Create a new JSON file in `arld/data/library/` conforming to the `AircraftLibraryEntry` schema (see `arld/schemas/`).
2. Source or create an original SVG silhouette scaled to the published wingspan.
3. Update `library_manifest.json` with the new entry ID and its SHA-256 file hash.
4. Run the schema validation CI job locally to confirm the entry parses without error.
5. Cross-reference dimensional data against at least three published sources; note them in the `data_source` field.

IDs are permanent — never reassign or reuse a retired entry ID.

---

## Project File Format

Project files use the `.arld` extension (UTF-8 JSON). Schema is at `arld/schemas/arld-project.schema.json`.

### Schema Migration

When `schema_version` is incremented:
1. Write a migration function in `arld/core/ProjectMigration.cpp`.
2. The application auto-migrates on file open and prompts the user to save in the new version.
3. Update `arld/schemas/arld-project.schema.json` and add the new version to the CI schema-validate job.

---

## Packaging and Release

> Packaging commands will be populated during Phase 1 once the installer scaffolding is established.

### macOS
```bash
# TBD — CPack .dmg target
cmake --build --preset mac-release --target package
```

### Linux
```bash
# TBD — CPack .AppImage and .deb targets
cmake --build --preset linux-release --target package
```

### Windows
```bash
# TBD — CPack .msi target via WiX Toolset
cmake --build --preset win-release --target package
```

---

## Phase Gate Checklists

### Phase 0 → Phase 1 Gate (PoC Sign-Off)

- [ ] GitHub Actions CI passes clean builds on all 3 matrix targets
- [ ] 20 aircraft types in library with correct scaled silhouette rendering
- [ ] Test layout: 15 aircraft created, saved, reloaded, and SVG-exported with no data loss
- [ ] Clearance violations correctly detected for ≥ 5 scenarios defined in the test suite
- [ ] Domain expert (airshow ramp coordinator) completes a 10-aircraft layout in < 20 minutes unassisted
- [ ] No GPL-licensed code in deliverable binaries (license-check CI step passes)

### Phase 3 → Phase 4 Gate (SaaS Initiation)

- [ ] ≥ 50 active airshow events using ARLD v1.0 for primary ramp planning
- [ ] Net Promoter Score (NPS) ≥ 40 from show directors (6 months post-launch)
- [ ] Written Phase 4 business case approved by steering committee with dedicated funding

---

## Sprint Cadence

- Sprint duration: 2 weeks
- Story sizing: Fibonacci (1, 2, 3, 5, 8, 13)
- Velocity targets: 28–32 pts/sprint (Phase 0) → 42–50 pts/sprint (Phases 1–2)

**Definition of Done** for each story:
1. Code merged via reviewed PR with no unresolved comments
2. Catch2 tests added; `core/` coverage remains ≥ 80%
3. CI passes all 5 jobs on all 3 platforms
4. Any new user-visible string wrapped in `tr()` and added to the `.ts` translation file
5. ADR updated if story introduces a new design pattern or library
