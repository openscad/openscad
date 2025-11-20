# OpenSCAD MSVC Build Script for Local Windows Development
# This script replicates the GitHub Actions MSVC workflow for local building
#
# This script will automatically install missing prerequisites using winget
# Run with -InstallPrerequisites to install all required software automatically

param(
    [string]$SourceDir = "",  # If empty, will auto-detect repository root
    [string]$BuildDir = "",   # If empty, will use $SourceDir\build\windows-msvc-release
    [string]$Qt6Version = "6.10.0",
    [string]$Qt6Path = "",  # If empty, will try to detect or prompt
    [switch]$InstallPrerequisites,  # Install missing prerequisites automatically
    [switch]$SkipQScintilla,
    [switch]$SkipVcpkg,
    [switch]$SkipBuild,
    [switch]$RunTests,
    [switch]$CleanBuild
)

$ErrorActionPreference = "Stop"

# Auto-detect repository root if not specified
if ([string]::IsNullOrEmpty($SourceDir)) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    # Check if we're in the scripts directory
    if ((Split-Path -Leaf $scriptDir) -eq "scripts") {
        $SourceDir = Split-Path -Parent $scriptDir
    } else {
        $SourceDir = $scriptDir
    }
}

# Set BuildDir if not specified
if ([string]::IsNullOrEmpty($BuildDir)) {
    $BuildDir = "$SourceDir\build\windows-msvc-release"
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "OpenSCAD MSVC Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Function to check if a command exists
function Test-Command {
    param($Command)
    $null = Get-Command $Command -ErrorAction SilentlyContinue
    return $?
}

# Function to install a package using winget
function Install-WithWinget {
    param(
        [string]$PackageId,
        [string]$PackageName
    )

    Write-Host "Installing $PackageName via winget..." -ForegroundColor Yellow
    winget install --id $PackageId --silent --accept-source-agreements --accept-package-agreements

    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ $PackageName installed successfully" -ForegroundColor Green
        return $true
    } else {
        Write-Host "⚠ Failed to install $PackageName (exit code: $LASTEXITCODE)" -ForegroundColor Yellow
        return $false
    }
}

# Install prerequisites if requested
if ($InstallPrerequisites) {
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Installing Prerequisites" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""

    # Check if winget is available
    if (-not (Test-Command winget)) {
        Write-Host "ERROR: winget is not available on this system." -ForegroundColor Red
        Write-Host "winget is included with Windows 10 1809+ and Windows 11." -ForegroundColor Yellow
        Write-Host "Please update Windows or install App Installer from the Microsoft Store." -ForegroundColor Yellow
        exit 1
    }

    Write-Host "Using winget to install missing prerequisites..." -ForegroundColor Yellow
    Write-Host ""

    # Install Git
    if (-not (Test-Command git)) {
        Install-WithWinget -PackageId "Git.Git" -PackageName "Git"
    } else {
        Write-Host "✓ Git already installed" -ForegroundColor Green
    }

    # Install CMake
    if (-not (Test-Command cmake)) {
        Install-WithWinget -PackageId "Kitware.CMake" -PackageName "CMake"
    } else {
        Write-Host "✓ CMake already installed" -ForegroundColor Green
    }

    # Install Visual Studio Build Tools (or check if VS is installed)
    $vsPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsInstalled = $false

    if (Test-Path $vsPath) {
        $vs = & $vsPath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vs) {
            Write-Host "✓ Visual Studio with C++ tools already installed at: $vs" -ForegroundColor Green
            $vsInstalled = $true
        }
    }

    if (-not $vsInstalled) {
        Write-Host "Visual Studio 2022 with C++ tools not found." -ForegroundColor Yellow
        Write-Host "You have two options:" -ForegroundColor Yellow
        Write-Host "  1. Install Visual Studio 2022 Community (full IDE)" -ForegroundColor Gray
        Write-Host "  2. Install Visual Studio 2022 Build Tools (command-line only)" -ForegroundColor Gray
        Write-Host ""
        $vsChoice = Read-Host "Enter choice (1 or 2, or S to skip)"

        if ($vsChoice -eq "1") {
            Write-Host "Installing Visual Studio 2022 Community..." -ForegroundColor Yellow
            Write-Host "Note: You'll need to manually select 'Desktop development with C++' workload during installation." -ForegroundColor Yellow
            winget install --id Microsoft.VisualStudio.2022.Community --silent --override "--quiet --wait --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
        } elseif ($vsChoice -eq "2") {
            Write-Host "Installing Visual Studio 2022 Build Tools..." -ForegroundColor Yellow
            winget install --id Microsoft.VisualStudio.2022.BuildTools --silent --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
        } else {
            Write-Host "⚠ Skipping Visual Studio installation" -ForegroundColor Yellow
        }
    }

    # Install MSYS2
    if (-not (Test-Path "C:\msys64\usr\bin\bash.exe")) {
        Install-WithWinget -PackageId "MSYS2.MSYS2" -PackageName "MSYS2"
    } else {
        Write-Host "✓ MSYS2 already installed" -ForegroundColor Green
    }

    # Install Ghostscript (needed for PDF export tests)
    if (-not (Test-Command gs)) {
        Install-WithWinget -PackageId "Ghostscript.Ghostscript" -PackageName "Ghostscript"
    } else {
        Write-Host "✓ Ghostscript already installed" -ForegroundColor Green
    }

    # Note about Mesa (software OpenGL rendering)
    Write-Host ""
    Write-Host "Mesa Software OpenGL Rendering:" -ForegroundColor Yellow
    Write-Host "For running tests without GPU acceleration (similar to CI), you can install Mesa." -ForegroundColor Yellow
    Write-Host "Download from: https://github.com/pal1000/mesa-dist-win/releases" -ForegroundColor Gray
    Write-Host "Version 24.2.3 or newer recommended (release-msvc build)" -ForegroundColor Gray
    Write-Host "This is optional for local development but required for headless CI." -ForegroundColor Gray
    Write-Host ""

    # Install Qt6
    if ([string]::IsNullOrEmpty($Qt6Path)) {
        Write-Host ""
        Write-Host "Qt6 Installation:" -ForegroundColor Yellow
        Write-Host "Qt6 needs to be installed manually via the official Qt installer." -ForegroundColor Yellow
        Write-Host "Please install Qt 6.10.0 with the following components:" -ForegroundColor Yellow
        Write-Host "  - MSVC 2022 64-bit" -ForegroundColor Gray
        Write-Host "  - Qt 5 Compatibility Module" -ForegroundColor Gray
        Write-Host "  - Qt Multimedia" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Download from: https://www.qt.io/download-qt-installer" -ForegroundColor Cyan
        Write-Host ""

        $installQt = Read-Host "Press Enter after installing Qt6, or S to skip"
        if ($installQt -ne "S" -and $installQt -ne "s") {
            Write-Host "Continuing with Qt6 installed..." -ForegroundColor Green
        }
    }

    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Prerequisites Installation Complete" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "NOTE: You may need to restart your terminal or computer for PATH changes to take effect." -ForegroundColor Yellow
    Write-Host ""

    $restart = Read-Host "Do you want to restart PowerShell now? (Y/N)"
    if ($restart -eq "Y" -or $restart -eq "y") {
        Write-Host "Please close this window and open a new PowerShell window, then run the script again without -InstallPrerequisites" -ForegroundColor Yellow
        exit 0
    }
}

# Check prerequisites
Write-Host "Checking prerequisites..." -ForegroundColor Yellow

if (-not (Test-Command cmake)) {
    Write-Host "ERROR: CMake not found. Please install CMake 3.20+" -ForegroundColor Red
    exit 1
}

if (-not (Test-Command git)) {
    Write-Host "ERROR: Git not found. Please install Git" -ForegroundColor Red
    exit 1
}

Write-Host "✓ Prerequisites check passed" -ForegroundColor Green
Write-Host ""

# Initialize submodules
Write-Host "Initializing Git submodules..." -ForegroundColor Yellow
git submodule update --init --recursive
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Git submodule initialization failed" -ForegroundColor Red
    exit 1
}
Write-Host "✓ Submodules initialized" -ForegroundColor Green
Write-Host ""

# Setup Visual Studio environment
Write-Host "Setting up Visual Studio environment..." -ForegroundColor Yellow
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsPath) {
        $vcvarsPath = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $vcvarsPath) {
            Write-Host "Found Visual Studio at: $vsPath" -ForegroundColor Green
            # Import VS environment variables using full path to Windows cmd.exe
            & $env:ComSpec /c """$vcvarsPath"" && set" | ForEach-Object {
                if ($_ -match "^([^=]+)=(.*)$") {
                    [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
                }
            }
        }
    }
}
Write-Host "✓ Visual Studio environment configured" -ForegroundColor Green
Write-Host ""

# Find or prompt for Qt6
if ([string]::IsNullOrEmpty($Qt6Path)) {
    Write-Host "Searching for Qt6 installation..." -ForegroundColor Yellow

    # Common Qt installation paths
    $possiblePaths = @(
        "C:\Qt\$Qt6Version\msvc2022_64",
        "C:\Qt\$Qt6Version\msvc2019_64",
        "$env:USERPROFILE\Qt\$Qt6Version\msvc2022_64",
        "D:\Qt\$Qt6Version\msvc2022_64"
    )

    foreach ($path in $possiblePaths) {
        if (Test-Path "$path\bin\qmake.exe") {
            $Qt6Path = $path
            Write-Host "Found Qt6 at: $Qt6Path" -ForegroundColor Green
            break
        }
    }

    if ([string]::IsNullOrEmpty($Qt6Path)) {
        Write-Host "Qt6 not found automatically. Please enter Qt6 installation path:" -ForegroundColor Yellow
        Write-Host "Example: C:\Qt\6.10.0\msvc2022_64" -ForegroundColor Gray
        $Qt6Path = Read-Host "Qt6 Path"

        if (-not (Test-Path "$Qt6Path\bin\qmake.exe")) {
            Write-Host "ERROR: Invalid Qt6 path. qmake.exe not found at $Qt6Path\bin" -ForegroundColor Red
            exit 1
        }
    }
}

$env:Qt6_DIR = $Qt6Path
$env:QT_ROOT_DIR = $Qt6Path
$env:CMAKE_PREFIX_PATH = $Qt6Path
$env:PATH = "$Qt6Path\bin;$env:PATH"
Write-Host "✓ Qt6 configured: $Qt6Path" -ForegroundColor Green
Write-Host ""

# Build QScintilla
if (-not $SkipQScintilla) {
    $qsciLib = "$Qt6Path\lib\qscintilla2_qt6.lib"
    $qsciDll = "$Qt6Path\lib\qscintilla2_qt6.dll"
    $qsciInclude = "$Qt6Path\include\Qsci"

    if ((Test-Path $qsciLib) -and (Test-Path $qsciDll) -and (Test-Path $qsciInclude)) {
        Write-Host "QScintilla already installed, skipping build" -ForegroundColor Green
    } else {
        Write-Host "Building QScintilla from source..." -ForegroundColor Yellow

        $qsciUrl = "https://www.riverbankcomputing.com/static/Downloads/QScintilla/2.14.1/QScintilla_src-2.14.1.tar.gz"
        $qsciArchive = "$env:TEMP\qscintilla.tar.gz"

        Write-Host "  Downloading QScintilla..."
        Invoke-WebRequest -Uri $qsciUrl -OutFile $qsciArchive

        Write-Host "  Extracting QScintilla..."
        tar -xzf $qsciArchive -C $env:TEMP
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERROR: Failed to extract QScintilla archive" -ForegroundColor Red
            exit 1
        }

        Push-Location "$env:TEMP\QScintilla_src-2.14.1\src"
        try {
            Write-Host "  Building QScintilla..."
            qmake qscintilla.pro
            if ($LASTEXITCODE -ne 0) {
                Write-Host "ERROR: QScintilla qmake failed" -ForegroundColor Red
                exit 1
            }

            nmake
            if ($LASTEXITCODE -ne 0) {
                Write-Host "ERROR: QScintilla build failed" -ForegroundColor Red
                exit 1
            }

            Write-Host "  Installing QScintilla..."
            nmake install
            if ($LASTEXITCODE -ne 0) {
                Write-Host "ERROR: QScintilla installation failed" -ForegroundColor Red
                exit 1
            }

            Write-Host "✓ QScintilla built and installed successfully" -ForegroundColor Green
        } finally {
            Pop-Location
            Remove-Item -Recurse -Force "$env:TEMP\QScintilla_src-2.14.1" -ErrorAction SilentlyContinue
            Remove-Item -Force $qsciArchive -ErrorAction SilentlyContinue
        }
    }
    Write-Host ""
}

# Setup MSYS2 (for flex and bison)
Write-Host "Checking MSYS2..." -ForegroundColor Yellow
$msys2Path = "C:\msys64\usr\bin"
if (Test-Path $msys2Path) {
    $env:PATH = "$msys2Path;$env:PATH"

    if (-not (Test-Path "$msys2Path\flex.exe") -or -not (Test-Path "$msys2Path\bison.exe")) {
        Write-Host "  Installing flex and bison via pacman..."
        & C:\msys64\usr\bin\pacman.exe -Sy --noconfirm flex bison
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERROR: Failed to install flex and bison via pacman" -ForegroundColor Red
            exit 1
        }
    }
    Write-Host "✓ MSYS2 configured (flex and bison available)" -ForegroundColor Green
} else {
    Write-Host "WARNING: MSYS2 not found at $msys2Path" -ForegroundColor Yellow
    Write-Host "         You may need to install flex and bison manually" -ForegroundColor Yellow
}
Write-Host ""

# Install 7-Zip if not available (needed for Mesa extraction)
if (-not (Test-Command 7z)) {
    Write-Host "Installing 7-Zip..." -ForegroundColor Yellow
    $7zipInstaller = "$env:TEMP\7z-install.exe"
    $7zipUrl = "https://www.7-zip.org/a/7z2408-x64.exe"

    Invoke-WebRequest -Uri $7zipUrl -OutFile $7zipInstaller

    # Install silently
    Start-Process -FilePath $7zipInstaller -ArgumentList "/S" -Wait -NoNewWindow

    # Add 7-Zip to PATH for this session
    $7zipPath = "${env:ProgramFiles}\7-Zip"
    if (Test-Path $7zipPath) {
        $env:PATH = "$7zipPath;$env:PATH"
    }

    Remove-Item -Force $7zipInstaller -ErrorAction SilentlyContinue

    # Verify installation
    if (Test-Command 7z) {
        Write-Host "✓ 7-Zip installed successfully" -ForegroundColor Green
    } else {
        Write-Host "ERROR: Failed to install 7-Zip" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "✓ 7-Zip already installed" -ForegroundColor Green
}
Write-Host ""

# Install Mesa for software OpenGL rendering
Write-Host "Setting up Mesa (software OpenGL)..." -ForegroundColor Yellow
$mesaVersion = "24.2.7"
$mesaInstallDir = "$env:LOCALAPPDATA\mesa"
$mesaUrl = "https://github.com/pal1000/mesa-dist-win/releases/download/$mesaVersion/mesa3d-$mesaVersion-release-msvc.7z"
$mesaArchive = "$env:TEMP\mesa.7z"

if (-not (Test-Path "$mesaInstallDir\x64\opengl32.dll")) {
    Write-Host "  Downloading Mesa $mesaVersion..."
    Invoke-WebRequest -Uri $mesaUrl -OutFile $mesaArchive
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to download Mesa" -ForegroundColor Red
        exit 1
    }

    Write-Host "  Extracting Mesa..."
    if (-not (Test-Path $mesaInstallDir)) {
        New-Item -ItemType Directory -Path $mesaInstallDir -Force | Out-Null
    }

    # Extract using 7-Zip
    & 7z x $mesaArchive "-o$mesaInstallDir" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to extract Mesa archive" -ForegroundColor Red
        exit 1
    }

    # Verify extraction succeeded
    if (-not (Test-Path "$mesaInstallDir\x64\opengl32.dll")) {
        Write-Host "ERROR: Mesa extraction failed - opengl32.dll not found" -ForegroundColor Red
        exit 1
    }

    Remove-Item -Force $mesaArchive -ErrorAction SilentlyContinue
    Write-Host "✓ Mesa installed to $mesaInstallDir" -ForegroundColor Green
} else {
    Write-Host "✓ Mesa already installed at $mesaInstallDir" -ForegroundColor Green
}

# Add Mesa to PATH for runtime
$env:PATH = "$mesaInstallDir\x64;$env:PATH"
Write-Host ""

# Setup vcpkg
if (-not $SkipVcpkg) {
    Write-Host "Setting up vcpkg..." -ForegroundColor Yellow

    $vcpkgDir = "$SourceDir\vcpkg"
    $vcpkgExe = "$vcpkgDir\vcpkg.exe"

    # Clone vcpkg if it doesn't exist
    if (-not (Test-Path $vcpkgDir)) {
        Write-Host "  Cloning vcpkg from GitHub..."
        git clone https://github.com/Microsoft/vcpkg.git $vcpkgDir
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERROR: Failed to clone vcpkg repository" -ForegroundColor Red
            exit 1
        }

        # Checkout specific commit used in CI (optional but ensures consistency)
        Push-Location $vcpkgDir
        try {
            git checkout 74e6536215718009aae747d86d84b78376bf9e09
            if ($LASTEXITCODE -ne 0) {
                Write-Host "ERROR: Failed to checkout vcpkg commit" -ForegroundColor Red
                exit 1
            }
        } finally {
            Pop-Location
        }
    }

    # Bootstrap vcpkg if not already done
    if (-not (Test-Path $vcpkgExe)) {
        Write-Host "  Bootstrapping vcpkg..."
        Push-Location $vcpkgDir
        try {
            .\bootstrap-vcpkg.bat -disableMetrics
            if ($LASTEXITCODE -ne 0) {
                Write-Host "ERROR: vcpkg bootstrap failed" -ForegroundColor Red
                exit 1
            }
        } finally {
            Pop-Location
        }
    }

    Write-Host "  Installing vcpkg dependencies from vcpkg.json..."
    $env:VCPKG_ROOT = $vcpkgDir

    # Change to source directory so vcpkg can find vcpkg.json
    Push-Location $SourceDir
    try {
        & $vcpkgExe install --triplet x64-windows
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERROR: vcpkg dependency installation failed" -ForegroundColor Red
            exit 1
        }
    } finally {
        Pop-Location
    }

    Write-Host "✓ vcpkg dependencies installed" -ForegroundColor Green
    Write-Host ""

    # Install Python dependencies using vcpkg Python
    Write-Host "Installing Python dependencies..." -ForegroundColor Yellow
    $vcpkgPython = "$vcpkgDir\installed\x64-windows\tools\python3\python.exe"

    if (-not (Test-Path $vcpkgPython)) {
        Write-Host "ERROR: vcpkg Python not found at $vcpkgPython" -ForegroundColor Red
        Write-Host "       Make sure 'python3' is in vcpkg.json dependencies" -ForegroundColor Red
        exit 1
    }

    Write-Host "  Using Python: $vcpkgPython" -ForegroundColor Gray

    & $vcpkgPython -m pip install --upgrade pip
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to upgrade pip" -ForegroundColor Red
        exit 1
    }

    & $vcpkgPython -m pip install bsdiff4 numpy pillow
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to install Python dependencies" -ForegroundColor Red
        exit 1
    }
    Write-Host "✓ Python dependencies installed" -ForegroundColor Green
    Write-Host ""
}

# Clean build directory if requested
if ($CleanBuild -and (Test-Path $BuildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
    Write-Host "✓ Build directory cleaned" -ForegroundColor Green
    Write-Host ""
}

# Configure and build with CMake
if (-not $SkipBuild) {
    Write-Host "Configuring CMake..." -ForegroundColor Yellow

    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
    }

    cmake --preset windows-msvc-release
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
        exit 1
    }

    Write-Host "✓ CMake configuration complete" -ForegroundColor Green
    Write-Host ""

    Write-Host "Building OpenSCAD..." -ForegroundColor Yellow
    cmake --build --preset windows-msvc-release --verbose
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Build failed" -ForegroundColor Red
        exit 1
    }

    Write-Host "✓ Build complete" -ForegroundColor Green
    Write-Host ""

    # Copy Qt and dependency DLLs next to executable
    Write-Host "Copying runtime DLLs..." -ForegroundColor Yellow
    $releaseDir = "$BuildDir\Release"

    # Copy Qt DLLs
    if (Test-Path "$Qt6Path\bin") {
        Copy-Item "$Qt6Path\bin\*.dll" -Destination $releaseDir -ErrorAction SilentlyContinue
    }
    if (Test-Path "$Qt6Path\lib") {
        Copy-Item "$Qt6Path\lib\*.dll" -Destination $releaseDir -ErrorAction SilentlyContinue
    }

    # Copy vcpkg DLLs
    $vcpkgBinDir = "$BuildDir\vcpkg_installed\x64-windows\bin"
    if (Test-Path $vcpkgBinDir) {
        Copy-Item "$vcpkgBinDir\*.dll" -Destination $releaseDir -ErrorAction SilentlyContinue
    }

    # Copy Mesa DLLs for software OpenGL rendering
    $mesaInstallDir = "$env:LOCALAPPDATA\mesa"
    if (Test-Path "$mesaInstallDir\x64") {
        Copy-Item "$mesaInstallDir\x64\*.dll" -Destination $releaseDir -ErrorAction SilentlyContinue
    }

    Write-Host "✓ Runtime DLLs copied" -ForegroundColor Green
    Write-Host ""
}

# Run tests if requested
if ($RunTests) {
    Write-Host "Running tests..." -ForegroundColor Yellow

    # Set Python environment variables for tests
    $pythonToolsDir = "$BuildDir\vcpkg_installed\x64-windows\tools\python3"
    if (Test-Path $pythonToolsDir) {
        $env:PYTHONHOME = $pythonToolsDir
        $env:PYTHONPATH = "$pythonToolsDir\Lib;$pythonToolsDir\Lib\site-packages"
    }

    # Add Qt and Mesa to PATH for tests
    $mesaInstallDir = "$env:LOCALAPPDATA\mesa"
    $env:PATH = "$Qt6Path\bin;$Qt6Path\lib;$mesaInstallDir\x64;$env:PATH"

    Push-Location $BuildDir
    try {
        ctest -C Release -j2
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERROR: Tests failed" -ForegroundColor Red
            exit 1
        }
        Write-Host "✓ Tests complete" -ForegroundColor Green
    } finally {
        Pop-Location
    }
    Write-Host ""
}

# Summary
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Source Directory: $SourceDir" -ForegroundColor White
Write-Host "Build Directory:  $BuildDir" -ForegroundColor White
Write-Host "Qt6 Path:         $Qt6Path" -ForegroundColor White
Write-Host ""

if (Test-Path "$BuildDir\Release\openscad.exe") {
    Write-Host "✓ Executable: $BuildDir\Release\openscad.exe" -ForegroundColor Green
} else {
    Write-Host "⚠ Executable not found (build may have failed)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "To run tests manually:" -ForegroundColor Yellow
Write-Host "  cd $BuildDir\tests" -ForegroundColor Gray
Write-Host "  ctest -C Release --output-on-failure" -ForegroundColor Gray
Write-Host ""
Write-Host "To run OpenSCAD:" -ForegroundColor Yellow
Write-Host "  $BuildDir\Release\openscad.exe" -ForegroundColor Gray
Write-Host ""
