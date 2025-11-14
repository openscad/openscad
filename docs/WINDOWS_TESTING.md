# Running Tests on Windows (MSVC)

This guide explains how to run OpenSCAD tests on Windows with MSVC builds.

## Prerequisites

### 1. Mesa Software OpenGL Renderer

OpenSCAD tests require OpenGL support with Framebuffer Objects (FBO). On Windows systems without GPU access or when running headless, you need Mesa's software renderer.

#### Automatic Installation (Recommended)

Mesa is now included as a vcpkg dependency and will be automatically installed when you build OpenSCAD:

1. Rebuild with vcpkg to install Mesa:
   ```powershell
   # Clean build to ensure Mesa is installed
   .\scripts\build-msvc-local.ps1 -CleanBuild
   ```

2. Verify Mesa DLL was copied to Release directory:
   ```powershell
   Test-Path "build\windows-msvc-release\Release\opengl32.dll"
   ```

#### Manual Installation (Alternative)

If you prefer to install Mesa manually or the vcpkg version doesn't work:

1. Download Mesa from: https://github.com/pal1000/mesa-dist-win/releases
   - Recommended version: **24.2.3** or newer
   - Download the **release-msvc** build (not mingw)

2. Extract the archive

3. Copy `opengl32.dll` from the `x64` folder to your OpenSCAD Release directory:
   ```powershell
   Copy-Item "path\to\mesa\x64\opengl32.dll" -Destination "build\windows-msvc-release\Release\"
   ```

### 2. Qt Platform Plugins

Qt needs platform plugins to initialize properly. These are automatically copied by the setup script.

### 3. Fontconfig Configuration

Fontconfig needs to know where to find its configuration files. This is handled by environment variables.

## Running Tests

### Automated Setup and Test Run

Use the provided test runner script:

```powershell
# Run all tests
.\scripts\run-tests.ps1

# Run tests with specific filter
.\scripts\run-tests.ps1 -TestFilter "render-manifold_*"

# Run with verbose output
.\scripts\run-tests.ps1 -Verbose

# Run with custom number of parallel jobs
.\scripts\run-tests.ps1 -Jobs 8
```

### Manual Setup

If you prefer to set up the environment manually:

```powershell
# 1. Set up test environment (copies Qt plugins, sets env vars)
.\scripts\setup-test-environment.ps1

# 2. Set environment variables in your PowerShell session
$BuildDir = "build\windows-msvc-release"
$VcpkgDir = "$BuildDir\vcpkg_installed\x64-windows"
$env:FONTCONFIG_FILE = "$VcpkgDir\etc\fonts\fonts.conf"
$env:FONTCONFIG_PATH = "$VcpkgDir\etc\fonts"
$env:QT_OPENGL = "software"
$env:QT_QPA_PLATFORM = "offscreen"

# 3. Add Qt to PATH
$env:PATH = "C:\Qt\6.10.0\msvc2022_64\bin;C:\Qt\6.10.0\msvc2022_64\lib;$env:PATH"

# 4. Run tests
cd build\windows-msvc-release
ctest -C Release --output-on-failure -j $env:NUMBER_OF_PROCESSORS
```

## Troubleshooting

### "Framebuffer Objects not supported" Error

**Symptom:**
```
ERROR: Framebuffer Objects not supported
Can't create OffscreenView: Unable to create FBO.
```

**Cause:** Mesa OpenGL renderer is not installed or not being used.

**Solution:**
1. Install Mesa as described above
2. Verify `opengl32.dll` exists in the Release directory
3. If using system-wide deployment, restart your terminal
4. Try setting `QT_OPENGL=software` explicitly

### "Fontconfig error: Cannot load default config file"

**Symptom:**
```
Fontconfig error: Cannot load default config file: No such file: (null)
```

**Cause:** Fontconfig environment variables are not set.

**Solution:**
- Use the `run-tests.ps1` script which sets them automatically
- Or manually set `FONTCONFIG_FILE` and `FONTCONFIG_PATH` as shown above

### Qt Platform Plugin Not Found

**Symptom:**
```
This application failed to start because no Qt platform plugin could be initialized.
```

**Cause:** Qt platform plugins directory is missing.

**Solution:**
- Run `.\scripts\setup-test-environment.ps1` to copy Qt plugins
- Or manually copy from `C:\Qt\6.10.0\msvc2022_64\plugins` to the Release directory

## Environment Variables Reference

| Variable | Purpose | Example Value |
|----------|---------|---------------|
| `FONTCONFIG_FILE` | Path to fontconfig main config | `build\windows-msvc-release\vcpkg_installed\x64-windows\etc\fonts\fonts.conf` |
| `FONTCONFIG_PATH` | Path to fontconfig directory | `build\windows-msvc-release\vcpkg_installed\x64-windows\etc\fonts` |
| `QT_OPENGL` | Tell Qt to use software rendering | `software` |
| `QT_QPA_PLATFORM` | Qt Platform Abstraction plugin | `offscreen` |

## CI vs Local Development

The GitHub Actions CI automatically installs Mesa using the `ssciwr/setup-mesa-dist-win` action. For local development, you need to manually install Mesa as described above.

The CI workflow (`.github/workflows/windows-msvc.yml`) can be used as a reference for the complete test setup process.
