# Local MSVC Build Instructions

This script allows you to build OpenSCAD locally on Windows 11 using MSVC, replicating the GitHub Actions workflow.

## Automatic Installation (Recommended)

The script can automatically install all prerequisites using Windows Package Manager (winget):

```powershell
# Install all prerequisites automatically
.\scripts\build-msvc-local.ps1 -InstallPrerequisites
```

This will install:

- Git
- CMake
- Python 3.11
- Visual Studio 2022 (Build Tools or Community Edition)
- MSYS2 (for flex/bison)
- Prompts you to manually install Qt6

After installation, restart your terminal and run the build:

```powershell
# Build after prerequisites are installed
.\scripts\build-msvc-local.ps1 -RunTests
```

## Manual Installation (Alternative)

If you prefer to install prerequisites manually:

1. **Visual Studio 2022** with "Desktop development with C++" workload
2. **CMake 3.20+** - [Download](https://cmake.org/download/)
3. **Git** with Git Bash - [Download](https://git-scm.com/download/win)
4. **Python 3.11+** - [Download](https://www.python.org/downloads/)
5. **Qt 6.10.0** - [Download](https://www.qt.io/download-qt-installer)
   - Install with: `msvc2022_64` component
   - Include modules: `qt5compat`, `qtmultimedia`
6. **MSYS2** (optional, for flex/bison) - [Download](https://www.msys2.org/)

## Quick Start (After Installing Prerequisites)

Open PowerShell in the openscad directory and run:

```powershell
# Full build with tests
.\scripts\build-msvc-local.ps1 -RunTests

# Clean build (removes previous build artifacts)
.\scripts\build-msvc-local.ps1 -CleanBuild

# Skip certain steps (if already done)
.\scripts\build-msvc-local.ps1 -SkipQScintilla -SkipVcpkg
```

## Script Options

- `-InstallPrerequisites` - Automatically install missing prerequisites using winget
- `-SourceDir` - Source directory (default: current directory)
- `-BuildDir` - Build output directory (default: `build\windows-msvc-release`)
- `-Qt6Path` - Path to Qt6 installation (auto-detected if not specified)
- `-SkipQScintilla` - Skip QScintilla build if already installed
- `-SkipVcpkg` - Skip vcpkg dependency installation
- `-SkipBuild` - Skip CMake configuration and build
- `-RunTests` - Run tests after building
- `-CleanBuild` - Remove build directory before building

## What the Script Does

1. ✓ Checks prerequisites (CMake, Git, Python)
2. ✓ Initializes Git submodules
3. ✓ Sets up Visual Studio environment (vcvars64.bat)
4. ✓ Detects or prompts for Qt6 installation
5. ✓ Builds QScintilla from source (if not cached)
6. ✓ Configures MSYS2 for flex/bison
7. ✓ Installs Python dependencies (bsdiff4, numpy, pillow)
8. ✓ Bootstraps vcpkg and installs dependencies
9. ✓ Configures CMake with windows-msvc-release preset
10. ✓ Builds OpenSCAD with verbose output
11. ✓ Optionally runs tests

## Troubleshooting

### Qt6 Not Found

If Qt6 is not auto-detected, the script will prompt you. Common paths:

- `C:\Qt\6.10.0\msvc2022_64`
- `%USERPROFILE%\Qt\6.10.0\msvc2022_64`

### MSYS2 Missing

If MSYS2 is not installed at `C:\msys64`, you may need to:

1. Install MSYS2 from <https://www.msys2.org/>
2. Or manually install flex/bison another way

### vcpkg Bootstrap Failed

If vcpkg fails to bootstrap, ensure Git submodules are initialized:

```powershell
git submodule update --init --recursive
```

### Build Failures

Check that you have:

1. Visual Studio 2022 with C++ tools installed
2. All prerequisites installed
3. Enough disk space (build can use several GB)

### Test Failures

If tests fail with DLL errors, ensure Qt6 bin directory is in PATH:

```powershell
$env:PATH = "C:\Qt\6.10.0\msvc2022_64\bin;$env:PATH"
```

## Manual Testing

After a successful build, you can run tests manually:

```powershell
cd build\windows-msvc-release\tests
ctest -C Release -L Default --output-on-failure
```

Or run a single test:

```powershell
ctest -C Release -L Default -R astdump_arg-permutations --output-on-failure -V
```

## Running OpenSCAD

After building:

```powershell
.\build\windows-msvc-release\Release\openscad.exe
```

Or with a file:

```powershell
.\build\windows-msvc-release\Release\openscad.exe examples\Basics\logo.scad
```
