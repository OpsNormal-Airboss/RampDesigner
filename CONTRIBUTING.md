# Contributing to ARLD

## Prerequisites

- **CMake** 3.28+
- **vcpkg** — set `VCPKG_ROOT` to your vcpkg installation directory
- **Qt 6.7** — install via [aqtinstall](https://github.com/miurahr/aqtinstall) or your system package manager
- **C++20 compiler**: Apple Clang 16+ (macOS), GCC 13+ (Linux), MSVC 2022 (Windows)
- **Ninja** build system

### Qt Installation

Qt 6.7 LGPL is required. The system Qt packages on Ubuntu 22.04 (`apt-get install qt6-base-dev`) ship Qt 6.4, which is too old — the binary will fail at runtime with `version 'Qt_6.7' not found`. Use one of the methods below for each platform.

**macOS** — Homebrew (tracks Qt 6.7+):

```bash
brew install qt
export Qt6_DIR=$(brew --prefix qt)/lib/cmake/Qt6   # add to ~/.zshrc
```

**Linux (Ubuntu 22.04 LTS)** — aqtinstall (do **not** use `apt-get install qt6-base-dev`):

```bash
pip3 install aqtinstall
aqt install-qt linux desktop 6.7.3 gcc_64 -O ~/Qt
```

Add the following to `~/.bashrc` (or `~/.zshrc`) so the build and the runtime both find Qt 6.7:

```bash
export Qt6_DIR=~/Qt/6.7.3/gcc_64/lib/cmake/Qt6
export LD_LIBRARY_PATH=~/Qt/6.7.3/gcc_64/lib
export QT_PLUGIN_PATH=~/Qt/6.7.3/gcc_64/plugins
```

Alternatively use the [Qt Online Installer](https://www.qt.io/download-qt-installer) — select Desktop → gcc 64-bit → Qt 6.7.x.

**Windows** — aqtinstall or the Qt Online Installer:

```bash
pip install aqtinstall
aqt install-qt windows desktop 6.7.3 win64_msvc2022_64 -O C:\Qt
```

Then pass `-DQt6_DIR=C:\Qt\6.7.3\msvc2022_64\lib\cmake\Qt6` to CMake, or set it as a system environment variable.

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
