# Building OpenSCAD with Visual Studio on Windows

This guide explains how to build OpenSCAD natively on Windows using Visual Studio and vcpkg.

## Prerequisites

### Required Software

1. **Visual Studio 2019 or 2022** with the following workloads:
   - Desktop development with C++
   - Download from: https://visualstudio.microsoft.com/

2. **CMake 3.21 or later**
   - Download from: https://cmake.org/download/
   - Make sure to add CMake to your system PATH during installation

3. **Git**
   - Download from: https://git-scm.com/download/win

4. **Python 3.9 or later**
   - Download from: https://www.python.org/downloads/
   - Make sure to check "Add Python to PATH" during installation

## Quick Start

### Automated Setup (Recommended)

1. Open PowerShell and navigate to the OpenSCAD directory:
   ```powershell
   cd path\to\openscad
   ```

2. Run the setup script:
   ```powershell
   .\scripts\setup-visualstudio-build.ps1
   ```

3. Build the project:
   ```powershell
   cmake --build --preset windows-msvc-release
   ```

4. Run tests:
   ```powershell
   ctest --preset windows-msvc-release --output-on-failure
   ```

### Manual Setup

If you prefer to set things up manually or want more control:

#### 1. Install vcpkg

```powershell
# Clone vcpkg (if not already installed)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Set environment variable
$env:VCPKG_ROOT = "C:\vcpkg"
```

#### 2. Install Python Dependencies

```powershell
pip install bsdiff4 numpy pillow
```

#### 3. Configure the Build

```powershell
cd path\to\openscad

# Configure using CMake preset
cmake --preset windows-msvc-release
```

This will automatically install all required dependencies via vcpkg using the `vcpkg.json` manifest file.

#### 4. Build

```powershell
cmake --build --preset windows-msvc-release --parallel
```

Or open the project in Visual Studio:
- Visual Studio → File → Open → CMake
- Select the `CMakeLists.txt` file
- Choose the `windows-msvc-release` configuration from the dropdown

#### 5. Run Tests

```powershell
ctest --preset windows-msvc-release --output-on-failure
```

## Build Configurations

Three CMake presets are available:

- `windows-msvc-debug` - Debug build with symbols
- `windows-msvc-release` - Optimized release build
- `windows-msvc-relwithdebinfo` - Release with debug symbols

To use a different configuration:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

## Troubleshooting

### vcpkg Dependency Installation Fails

If vcpkg fails to install dependencies:

1. Make sure you have internet connectivity
2. Try clearing the vcpkg cache:
   ```powershell
   Remove-Item -Recurse -Force $env:VCPKG_ROOT\buildtrees
   Remove-Item -Recurse -Force $env:VCPKG_ROOT\packages
   ```
3. Re-run the configure step

### CMake Can't Find Dependencies

Make sure the `VCPKG_ROOT` environment variable is set:

```powershell
$env:VCPKG_ROOT = "C:\vcpkg"  # or your vcpkg installation path
```

### Python Library Issues

If you encounter Python-related build errors:

1. Make sure Python is in your PATH
2. Install required Python packages:
   ```powershell
   pip install bsdiff4 numpy pillow
   ```

### Build Errors with Manifold/Clipper2

If you get errors related to submodules:

```powershell
git submodule update --init --recursive
```

## Advanced Options

### Custom vcpkg Location

If you have vcpkg installed in a custom location:

```powershell
.\scripts\setup-visualstudio-build.ps1 -VcpkgRoot "D:\my\vcpkg"
```

### Skip vcpkg Setup

If you already have dependencies installed:

```powershell
.\scripts\setup-visualstudio-build.ps1 -SkipVcpkg
```

### Debug Build

```powershell
.\scripts\setup-visualstudio-build.ps1 -BuildType Debug
```

## Differences from MSYS2 Build

The Visual Studio build differs from the MSYS2/MinGW build in several ways:

1. **Compiler**: Uses MSVC instead of GCC
2. **Dependencies**: Managed via vcpkg instead of MSYS2 packages
3. **Build System**: Native Windows tools instead of Unix-like environment
4. **Debugger**: Can use Visual Studio debugger directly

## CI/CD Integration

The project includes a GitHub Actions workflow for automated MSVC builds:

`.github/workflows/windows-msvc.yml`

This workflow:
- Sets up vcpkg with caching
- Builds OpenSCAD using the MSVC compiler
- Runs the test suite
- Uploads build artifacts

## Additional Resources

- [vcpkg Documentation](https://vcpkg.io/)
- [CMake Presets Documentation](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [Visual Studio CMake Documentation](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)

## Getting Help

If you encounter issues:

1. Check the [GitHub Issues](https://github.com/openscad/openscad/issues)
2. Review the build logs in `build/windows-msvc-*/`
3. Ensure all prerequisites are properly installed
