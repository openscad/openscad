# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OpenSCAD is a software for creating solid 3D CAD objects using a script-based approach. Unlike interactive 3D modelers, OpenSCAD reads script files and renders 3D models from them. The project is written in C++17 and uses CMake as its build system.

## Build System

### Configuration and Building

Use CMake with out-of-tree builds:

```bash
# Configure the build with experimental features enabled
cmake -B build -DEXPERIMENTAL=1

# Build the project (supports parallel builds with -j N)
cmake --build build -j 8

# Install (Linux only, requires root)
cmake --install build
```

### Common CMake Options

- `-DEXPERIMENTAL=1` - Enable experimental features (commonly used)
- `-DHEADLESS=ON` - Build without GUI frontend
- `-DNULLGL=ON` - Build without OpenGL (implies HEADLESS=ON)
- `-DSNAPSHOT=ON` - Create dev snapshot (uses nightly icons)
- `-DUSE_QT6=ON` - Build with Qt6 instead of Qt5 (default ON for macOS, OFF for others)
- `-DENABLE_TESTS=ON` - Run test suite after building (default ON)
- `-DUSE_CCACHE=ON` - Use ccache to speed up compilation (default ON)
- `-DCLANG_TIDY=1` - Enable clang-tidy static analysis

### Platform-Specific Build Scripts

**macOS:**
```bash
# Set environment variables first
source scripts/setenv-macos.sh

# Build dependencies from source
./scripts/macosx-build-dependencies.sh

# Or use Homebrew
./scripts/macosx-build-homebrew.sh
```

**Linux/BSD:**
```bash
# Install dependencies (uses system package manager)
sudo ./scripts/uni-get-dependencies.sh

# Check installed dependency versions
./scripts/check-dependencies.sh

# For older systems, build dependencies from source:
source ./scripts/setenv-unibuild.sh
./scripts/uni-build-dependencies.sh
```

**Windows (cross-compilation from Linux):**
```bash
# Set environment for 64-bit Windows build
source ./scripts/setenv-mingw-xbuild.sh 64

# Build dependencies (takes hours, requires tens of GB)
./scripts/mingw-x-build-dependencies.sh 64

# Create installer package
./scripts/release-common.sh mingw64
```

**WebAssembly:**
```bash
# Browser build (creates build-web/openscad.wasm and .js)
./scripts/wasm-base-docker-run.sh emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Debug -DEXPERIMENTAL=1
./scripts/wasm-base-docker-run.sh cmake --build build-web -j2

# Node.js standalone build
./scripts/wasm-base-docker-run.sh emcmake cmake -B build-node -DWASM_BUILD_TYPE=node -DCMAKE_BUILD_TYPE=Debug -DEXPERIMENTAL=1
./scripts/wasm-base-docker-run.sh cmake --build build-node -j2
```

## Testing

### Running Tests

```bash
# Run all tests from build directory (supports -j N for parallel execution)
cd build
ctest -j 8

# Test suite requires:
# - Python 3.8+
# - MCAD library (install via: git submodule update --init --recursive)
```

### Test Organization

Tests are located in `tests/` directory:
- `tests/data/` - Test data files including SCAD scripts
- `tests/regression/` - Regression test suite
- Python-based test runners:
  - `test_cmdline_tool.py` - Command-line interface tests
  - `test_pretty_print.py` - Pretty-printer tests
  - `stlexportsanitytest.py` - STL export validation
  - `export_import_pngtest.py` - PNG export/import tests

## Code Architecture

### Source Directory Structure

The codebase is organized into functional modules under `src/`:

- **`src/core/`** (~120 files) - Core language implementation
  - AST nodes and expression evaluation
  - Built-in functions and modules
  - Context management and variable scoping
  - Parser and lexer interfaces

- **`src/geometry/`** - Geometric operations and data structures
  - CSG (Constructive Solid Geometry) operations
  - 2D/3D geometry representations
  - CGAL integration (`geometry/cgal/`)
  - Clipper2 integration for 2D boolean operations
  - GeometryEvaluator - converts AST to geometry

- **`src/io/`** - File format import/export
  - 3MF, AMF, STL, DXF export
  - DXF import and dimension handling
  - Export format enumeration

- **`src/glview/`** - OpenGL rendering and visualization
  - Camera management
  - Frame buffer objects
  - Color mapping
  - CSG rendering

- **`src/gui/`** - Qt-based GUI application
  - Main window and editors
  - Preview and rendering UI
  - Settings and preferences

- **`src/platform/`** - Platform-specific code
  - Platform abstraction layer

- **`src/utils/`** - Utility functions and helpers

### Key Components

**Geometry Pipeline:**
1. Script parsing (lexer/parser) → AST
2. AST evaluation → Abstract CSG tree
3. GeometryEvaluator → Geometry objects
4. CGAL or Manifold backend → Mesh generation
5. Export or OpenGL rendering

**Submodules:**
- `submodules/MCAD/` - MCAD library (required for tests)
- `submodules/manifold/` - Manifold geometry kernel (optional)
- `submodules/Clipper2/` - 2D polygon clipping
- `submodules/OpenCSG/` - Constructive Solid Geometry rendering
- `submodules/mimalloc/` - Memory allocator

## Code Style and Quality

### Coding Standards

The project follows these conventions (enforced by `.uncrustify.cfg`):

- **Indentation:** 2 spaces (no tabs)
- **C++ Standard:** C++17 minimum, use C++11+ features where applicable
- **Style Guide:** See Scott Meyer's "Effective Modern C++" for modern C++ patterns

### Code Formatting

```bash
# Format changed files only (recommended)
scripts/beautify.sh

# Format entire codebase (use only for global updates)
scripts/beautify.sh --all
```

**Note:** Uses uncrustify commit `a05edf605a5b1ea69ac36918de563d4acf7f31fb` (Dec 24, 2017). Uncrustify versions may have breaking changes.

### Static Analysis

The project uses clang-tidy with configuration in `.clang-tidy`:

```bash
# Enable clang-tidy during CMake configuration
cmake -B build -DCLANG_TIDY=1 -DEXPERIMENTAL=1
```

Enabled check categories:
- `boost-*`
- `bugprone-*` (with some exclusions)
- `clang-analyzer-*` (with some exclusions)
- `misc-*` (with some exclusions)
- `modernize-*` (with some exclusions)
- `performance-*`

## Development Workflow

### Adding New Functions/Modules

1. Implement the feature
2. Add tests in `tests/`
3. For modules: Add example in `examples/`
4. Document:
   - Wikibooks documentation
   - Cheatsheet
   - For modules: Add tooltips in `src/gui/Editor.cc`
   - Update external editor modes
   - Add entry to `RELEASE_NOTES.md`

### Dependencies

Major dependencies (see README.md for minimum versions):
- **Build:** CMake (3.5+), C++17 compiler
- **Core:** Qt (5.12+/6.5+), Boost (1.61+), Eigen (3.x)
- **Geometry:** CGAL (5.4+), GMP, MPFR, OpenCSG (1.4.2+)
- **Graphics:** GLEW/GLAD, OpenGL
- **Text:** QScintilla2 (2.9+), fontconfig, freetype2, harfbuzz
- **Parser:** Bison (2.4+), Flex (2.5.35+)
- **Utilities:** libzip (0.10.1+), double-conversion (2.0.1+)

## CI/CD

### Running CI Locally

Install circleci-cli and run:

```bash
# Cross-compiled Windows builds
circleci local execute --job openscad-mxe-64bit
circleci local execute --job openscad-mxe-32bit

# Linux AppImage
circleci local execute --job openscad-appimage-64bit

# For interactive debugging:
docker run --entrypoint=/bin/bash -it openscad/mxe-x86_64-gui:latest
```

### GitHub Workflows

Located in `.github/workflows/`:
- `linux-build-matrix.yml` - Multi-configuration Linux builds
- `linux-tests.yml` - Linux test suite
- `macos-tests.yml` - macOS test suite
- `macos-release.yml` - macOS release builds
- `code-tidy.yml` - Clang-tidy static analysis
- `docker-build.yml` - Docker image builds
- `codeql-analysis.yml` - CodeQL security scanning

## Common Tasks

### Check Dependencies
```bash
./scripts/check-dependencies.sh
```

### Run Single Test
```bash
cd build
ctest -R <test_name_pattern>
```

### Generate Release Notes
```bash
./scripts/makereleasenotes.sh
```

### Create STL Files
```bash
./scripts/create-stl.sh
```

### Batch Processing
```bash
# Batch 2D exports
./scripts/batch-2d.sh

# Batch STL exports from examples
./scripts/batch-examples-stl.sh

# Batch STL exports from tests
./scripts/batch-tests-stl.sh
```

## Important Notes

- **Submodules:** Always initialize submodules: `git submodule update --init --recursive`
- **MCAD Required:** Tests require the MCAD library from submodules
- **Build Location:** Use out-of-tree builds (build directory separate from source)
- **Parallel Builds:** Both `cmake --build` and `ctest` support `-j N` for parallel execution
- **Platform Builds:** Windows builds are typically cross-compiled from Linux using MXE
- **WebAssembly:** Debug builds support C++ breakpoints in Firefox and Chrome (with extension)
