# Contributing to ARLD

## Prerequisites

- **CMake** 3.28+
- **vcpkg** — set `VCPKG_ROOT` to your vcpkg installation directory
- **Qt 6.7** — install via [aqtinstall](https://github.com/miurahr/aqtinstall) or your system package manager
- **C++20 compiler**: Apple Clang 16+ (macOS), GCC 13+ (Linux), MSVC 2022 (Windows)
- **Ninja** build system

### Qt Installation

```bash
# macOS
brew install qt@6

# Ubuntu 22.04
sudo apt-get install qt6-base-dev qt6-svg-dev

# Windows — use aqtinstall or the Qt online installer
pip install aqtinstall
aqt install-qt windows desktop 6.7.3 win64_msvc2022_64
```

## Building

```bash
# 1. Install C++ dependencies (CGAL, nlohmann-json, Catch2)
vcpkg install

# 2. Configure (choose your platform preset)
cmake --preset linux-debug     # or mac-debug / win-debug

# 3. Build
cmake --build --preset linux-debug

# 4. Run tests
ctest --preset linux-debug --output-on-failure
```

On Windows, run these commands from a **Developer Command Prompt for VS 2022** so MSVC is in `PATH`.

## Module Rules

- `arld/core/` must have **zero Qt dependency** — it must compile headless.
- All clearance distances live in `arld/core/include/arld/core/Config.h`. No magic numbers in `.cpp` files.
- All coordinates are stored in **feet**. Metric display values are derived at render time only.
- Every undoable action implements `ICommand` with `execute()`, `undo()`, and `describe()`.
- New dependencies require a license review — no GPL code in distributable binaries.

## Pull Request Checklist

- [ ] Code compiles on all 3 platforms (CI must be green)
- [ ] Catch2 tests added; `core/` coverage stays ≥ 80%
- [ ] No new magic numbers — constants go in `Config.h`
- [ ] No GPL transitive dependencies introduced
- [ ] Any new user-visible string wrapped in `tr()`
- [ ] ADR updated if a new design pattern or library is introduced

## License

By contributing you agree your changes will be licensed under Apache 2.0.
