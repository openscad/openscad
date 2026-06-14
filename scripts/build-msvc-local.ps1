# OpenSCAD MSVC Build Script for Local Windows Development
# This script replicates the GitHub Actions MSVC workflow for local building
#
# This script will automatically install missing prerequisites using winget
# Run with -InstallPrerequisites to install all required software automatically
#
# IMPORTANT: Windows Developer Mode is required to create symbolic links without admin rights.
#            Enable it via: Settings > Update & Security > For developers > Developer Mode
#            Or run PowerShell as Administrator
#
# Output Redirection:
#   All output:        .\scripts\build-msvc-local.ps1 -RunTests *>&1 > build.log
#   With Tee-Object:   .\scripts\build-msvc-local.ps1 -RunTests *>&1 | Tee-Object -FilePath build.log
#   Errors only:       .\scripts\build-msvc-local.ps1 -RunTests 2> errors.log
#   No warnings:       $WarningPreference='SilentlyContinue'; .\scripts\build-msvc-local.ps1 -RunTests

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
    [switch]$CleanBuild,
    [switch]$RerunFailedTests  # When combined with -RunTests, only reruns tests that previously failed
)

$ErrorActionPreference = "Stop"
$InformationPreference = "Continue"  # Make Write-Information visible by default

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

Write-Information "========================================"
Write-Information "OpenSCAD MSVC Build Script"
Write-Information "========================================"
Write-Output ""

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

    Write-Information "Installing $PackageName via winget..."
    winget install --id $PackageId --silent --accept-source-agreements --accept-package-agreements

    if ($LASTEXITCODE -eq 0) {
        Write-Information "✓ $PackageName installed successfully"
        return $true
    } else {
        Write-Warning " Failed to install $PackageName (exit code: $LASTEXITCODE)"
        return $false
    }
}

# Install prerequisites if requested
if ($InstallPrerequisites) {
    Write-Information "========================================"
    Write-Information "Installing Prerequisites"
    Write-Information "========================================"
    Write-Output ""

    # Check if winget is available
    if (-not (Test-Command winget)) {
        Write-Error "winget is not available on this system."
        Write-Information "winget is included with Windows 10 1809+ and Windows 11."
        Write-Information "Please update Windows or install App Installer from the Microsoft Store."
        exit 1
    }

    Write-Information "Using winget to install missing prerequisites..."
    Write-Output ""

    # Install Git
    if (-not (Test-Command git)) {
        Install-WithWinget -PackageId "Git.Git" -PackageName "Git"
    } else {
        Write-Information "✓ Git already installed"
    }

    # Install CMake
    if (-not (Test-Command cmake)) {
        Install-WithWinget -PackageId "Kitware.CMake" -PackageName "CMake"
    } else {
        Write-Information "✓ CMake already installed"
    }

    # Install Visual Studio Build Tools (or check if VS is installed)
    $vsPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsInstalled = $false

    if (Test-Path $vsPath) {
        $vs = & $vsPath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vs) {
            Write-Information "✓ Visual Studio with C++ tools already installed at: $vs"
            $vsInstalled = $true
        }
    }

    if (-not $vsInstalled) {
        Write-Information "Visual Studio 2022 with C++ tools not found."
        Write-Information "You have two options:"
        Write-Information "  1. Install Visual Studio 2022 Community (full IDE)"
        Write-Information "  2. Install Visual Studio 2022 Build Tools (command-line only)"
        Write-Output ""
        $vsChoice = Read-Host "Enter choice (1 or 2, or S to skip)"

        if ($vsChoice -eq "1") {
            Write-Information "Installing Visual Studio 2022 Community..."
            Write-Information "Note: You'll need to manually select 'Desktop development with C++' workload during installation."
            winget install --id Microsoft.VisualStudio.2022.Community --silent --override "--quiet --wait --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
        } elseif ($vsChoice -eq "2") {
            Write-Information "Installing Visual Studio 2022 Build Tools..."
            winget install --id Microsoft.VisualStudio.2022.BuildTools --silent --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
        } else {
            Write-Warning " Skipping Visual Studio installation"
        }
    }

    # Install MSYS2
    if (-not (Test-Path "C:\msys64\usr\bin\bash.exe")) {
        Install-WithWinget -PackageId "MSYS2.MSYS2" -PackageName "MSYS2"
    } else {
        Write-Information "✓ MSYS2 already installed"
    }

    # Install Ghostscript (needed for PDF export tests)
    if (-not (Test-Command gs)) {
        Install-WithWinget -PackageId "Ghostscript.Ghostscript" -PackageName "Ghostscript"
    } else {
        Write-Information "✓ Ghostscript already installed"
    }

    # Note about Mesa (software OpenGL rendering)
    Write-Output ""
    Write-Information "Mesa Software OpenGL Rendering:"
    Write-Information "For running tests without GPU acceleration (similar to CI), you can install Mesa."
    Write-Information "Download from: https://github.com/pal1000/mesa-dist-win/releases"
    Write-Information "Version 24.2.3 or newer recommended (release-msvc build)"
    Write-Information "This is optional for local development but required for headless CI."
    Write-Output ""

    # Install Qt6
    if ([string]::IsNullOrEmpty($Qt6Path)) {
        Write-Output ""
        Write-Information "Qt6 Installation:"
        Write-Information "Qt6 needs to be installed manually via the official Qt installer."
        Write-Information "Please install Qt 6.10.0 with the following components:"
        Write-Information "  - MSVC 2022 64-bit"
        Write-Information "  - Qt 5 Compatibility Module"
        Write-Information "  - Qt Multimedia"
        Write-Output ""
        Write-Information "Download from: https://www.qt.io/download-qt-installer"
        Write-Output ""

        $installQt = Read-Host "Press Enter after installing Qt6, or S to skip"
        if ($installQt -ne "S" -and $installQt -ne "s") {
            Write-Information "Continuing with Qt6 installed..."
        }
    }

    Write-Output ""
    Write-Information "========================================"
    Write-Information "Prerequisites Installation Complete"
    Write-Information "========================================"
    Write-Output ""
    Write-Information "NOTE: You may need to restart your terminal or computer for PATH changes to take effect."
    Write-Output ""

    $restart = Read-Host "Do you want to restart PowerShell now? (Y/N)"
    if ($restart -eq "Y" -or $restart -eq "y") {
        Write-Information "Please close this window and open a new PowerShell window, then run the script again without -InstallPrerequisites"
        exit 0
    }
}

# Check prerequisites
Write-Information "Checking prerequisites..."

if (-not (Test-Command cmake)) {
    Write-Error "CMake not found. Please install CMake 3.20+"
    exit 1
}

if (-not (Test-Command git)) {
    Write-Error "Git not found. Please install Git"
    exit 1
}

# Find native Windows CMake (not MSYS2's cmake which reports hostSystemName != Windows)
$cmakeExe = $null
foreach ($c in (Get-Command cmake -All -ErrorAction SilentlyContinue)) {
    if ($c.Source -notmatch 'msys64|msys2|cygwin') {
        $cmakeExe = $c.Source
        break
    }
}
if (-not $cmakeExe) {
    Write-Error "Native Windows CMake not found. MSYS2's cmake is not compatible with CMake presets on Windows."
    Write-Error "Install CMake from https://cmake.org or via winget: winget install Kitware.CMake"
    exit 1
}
Write-Information "✓ Prerequisites check passed (using cmake: $cmakeExe)"
Write-Output ""

# Initialize submodules
Write-Information "Initializing Git submodules..."
git submodule update --init --recursive
if ($LASTEXITCODE -ne 0) {
    Write-Error "Git submodule initialization failed"
    exit 1
}
Write-Information "✓ Submodules initialized"
Write-Output ""

# Setup Visual Studio environment
# Note: vcvars64.bat is NOT sourced here because it can interfere with
# environment variables needed later (VCPKG_ROOT, Qt paths, etc.).
# CMake with the "Visual Studio 17 2022" generator finds MSVC automatically
# via vswhere, so vcvars is not needed for the build.
Write-Information "Setting up Visual Studio environment..."
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsPath) {
        Write-Information "Found Visual Studio at: $vsPath"
    } else {
        Write-Error "Visual Studio with C++ tools not found."
        exit 1
    }
} else {
    Write-Error "vswhere.exe not found. Please install Visual Studio."
    exit 1
}
Write-Information "✓ Visual Studio environment configured"
Write-Output ""

# Find or prompt for Qt6
if ([string]::IsNullOrEmpty($Qt6Path)) {
    Write-Information "Searching for Qt6 installation..."

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
            Write-Information "Found Qt6 at: $Qt6Path"
            break
        }
    }

    if ([string]::IsNullOrEmpty($Qt6Path)) {
        Write-Information "Qt6 not found automatically. Please enter Qt6 installation path:"
        Write-Information "Example: C:\Qt\6.10.0\msvc2022_64"
        $Qt6Path = Read-Host "Qt6 Path"

        if (-not (Test-Path "$Qt6Path\bin\qmake.exe")) {
            Write-Error "Invalid Qt6 path. qmake.exe not found at $Qt6Path\bin"
            exit 1
        }
    }
}

$env:Qt6_DIR = $Qt6Path
$env:QT_ROOT_DIR = $Qt6Path
$env:CMAKE_PREFIX_PATH = $Qt6Path
$env:PATH = "$Qt6Path\bin;$env:PATH"
Write-Information "✓ Qt6 configured: $Qt6Path"
Write-Output ""

# Build QScintilla
if (-not $SkipQScintilla) {
    $qsciLib = "$Qt6Path\lib\qscintilla2_qt6.lib"
    $qsciDll = "$Qt6Path\lib\qscintilla2_qt6.dll"
    $qsciInclude = "$Qt6Path\include\Qsci"

    if ((Test-Path $qsciLib) -and (Test-Path $qsciDll) -and (Test-Path $qsciInclude)) {
        Write-Information "QScintilla already installed, skipping build"
    } else {
        Write-Information "Building QScintilla from source..."

        $qsciUrl = "https://www.riverbankcomputing.com/static/Downloads/QScintilla/2.14.1/QScintilla_src-2.14.1.tar.gz"
        $qsciArchive = "$env:TEMP\qscintilla.tar.gz"

        Write-Information "  Downloading QScintilla..."
        Invoke-WebRequest -Uri $qsciUrl -OutFile $qsciArchive

        Write-Information "  Extracting QScintilla..."
        tar -xzf $qsciArchive -C $env:TEMP
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Failed to extract QScintilla archive"
            exit 1
        }

        Push-Location "$env:TEMP\QScintilla_src-2.14.1\src"
        try {
            Write-Information "  Building QScintilla..."
            qmake qscintilla.pro
            if ($LASTEXITCODE -ne 0) {
                Write-Error "QScintilla qmake failed"
                exit 1
            }

            nmake
            if ($LASTEXITCODE -ne 0) {
                Write-Error "QScintilla build failed"
                exit 1
            }

            Write-Information "  Installing QScintilla..."
            nmake install
            if ($LASTEXITCODE -ne 0) {
                Write-Error "QScintilla installation failed"
                exit 1
            }

            Write-Information "✓ QScintilla built and installed successfully"
        } finally {
            Pop-Location
            Remove-Item -Recurse -Force "$env:TEMP\QScintilla_src-2.14.1" -ErrorAction SilentlyContinue
            Remove-Item -Force $qsciArchive -ErrorAction SilentlyContinue
        }
    }
    Write-Output ""
}

# Setup MSYS2 (for flex, bison, and ghostscript)
Write-Information "Checking MSYS2..."
$msys2Path = "C:\msys64\usr\bin"
$mingw64Path = "C:\msys64\mingw64\bin"
if (Test-Path $msys2Path) {
    # Append MSYS2 to PATH (not prepend) so system tools like cmake take priority.
    # MSYS2 provides flex, bison, ghostscript which aren't available elsewhere.
    $env:PATH = "$env:PATH;$msys2Path;$mingw64Path"

    Write-Information "  Installing/updating flex, bison, gettext, and ghostscript via pacman..."
    & C:\msys64\usr\bin\pacman.exe -S --noconfirm --needed flex bison gettext mingw-w64-x86_64-ghostscript
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to install MSYS2 packages via pacman"
        exit 1
    }
    Write-Information "✓ MSYS2 configured (flex, bison, gettext, and ghostscript available)"
} else {
    Write-Warning " MSYS2 not found at $msys2Path"
    Write-Information "         You may need to install flex, bison, and ghostscript manually"
}
Write-Output ""

# Install 7-Zip if not available (needed for Mesa extraction)
if (-not (Test-Command 7z)) {
    Write-Information "Installing 7-Zip..."
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
        Write-Information "✓ 7-Zip installed successfully"
    } else {
        Write-Error "Failed to install 7-Zip"
        exit 1
    }
} else {
    Write-Information "✓ 7-Zip already installed"
}
Write-Output ""

# Install Mesa for software OpenGL rendering
Write-Information "Setting up Mesa (software OpenGL)..."
$mesaVersion = "24.2.7"
$mesaInstallDir = "$env:LOCALAPPDATA\mesa"
$mesaUrl = "https://github.com/pal1000/mesa-dist-win/releases/download/$mesaVersion/mesa3d-$mesaVersion-release-msvc.7z"
$mesaArchive = "$env:TEMP\mesa.7z"

if (-not (Test-Path "$mesaInstallDir\x64\opengl32.dll")) {
    Write-Information "  Downloading Mesa $mesaVersion..."
    Invoke-WebRequest -Uri $mesaUrl -OutFile $mesaArchive
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to download Mesa"
        exit 1
    }

    Write-Information "  Extracting Mesa..."
    if (-not (Test-Path $mesaInstallDir)) {
        New-Item -ItemType Directory -Path $mesaInstallDir -Force | Out-Null
    }

    # Extract using 7-Zip
    & 7z x $mesaArchive "-o$mesaInstallDir" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to extract Mesa archive"
        exit 1
    }

    # Verify extraction succeeded
    if (-not (Test-Path "$mesaInstallDir\x64\opengl32.dll")) {
        Write-Error "Mesa extraction failed - opengl32.dll not found"
        exit 1
    }

    Remove-Item -Force $mesaArchive -ErrorAction SilentlyContinue
    Write-Information "✓ Mesa installed to $mesaInstallDir"
} else {
    Write-Information "✓ Mesa already installed at $mesaInstallDir"
}

# Add Mesa to PATH for runtime
$env:PATH = "$mesaInstallDir\x64;$env:PATH"
Write-Output ""

# Setup vcpkg
if (-not $SkipVcpkg) {
    Write-Information "Setting up vcpkg..."

    # Use system vcpkg (C:\vcpkg) or VCPKG_ROOT if set, fall back to repo-local
    $vcpkgDir = $null
    if ($env:VCPKG_ROOT -and (Test-Path "$env:VCPKG_ROOT\vcpkg.exe")) {
        $vcpkgDir = $env:VCPKG_ROOT
        Write-Information "  Using VCPKG_ROOT: $vcpkgDir"
    } elseif (Test-Path "C:\vcpkg\vcpkg.exe") {
        $vcpkgDir = "C:\vcpkg"
        Write-Information "  Using system vcpkg: $vcpkgDir"
    } elseif (Get-Command vcpkg -ErrorAction SilentlyContinue) {
        $vcpkgDir = Split-Path (Get-Command vcpkg).Source
        Write-Information "  Using vcpkg from PATH: $vcpkgDir"
    } else {
        Write-Error "vcpkg not found. Install vcpkg system-wide (e.g. git clone https://github.com/microsoft/vcpkg C:\vcpkg && C:\vcpkg\bootstrap-vcpkg.bat) or set VCPKG_ROOT."
        exit 1
    }

    $vcpkgExe = "$vcpkgDir\vcpkg.exe"

    Write-Information "  Installing vcpkg dependencies from vcpkg.json..."
    $env:VCPKG_ROOT = $vcpkgDir

    # Temporarily remove MSYS2 from PATH to prevent sh.exe from mangling
    # Windows paths (C: -> C;) during vcpkg's internal CMake invocations
    $savedPath = $env:PATH
    $env:PATH = ($env:PATH -split ';' | Where-Object { $_ -notmatch 'msys64' }) -join ';'
    $env:MSYS_NO_PATHCONV = "1"
    $env:MSYS2_ARG_CONV_EXCL = "*"
    
    # Set concurrency for parallel builds (use all available processors)
    if (-not $env:VCPKG_CONCURRENCY) {
        $env:VCPKG_CONCURRENCY = $env:NUMBER_OF_PROCESSORS
        Write-Information "  Set VCPKG_CONCURRENCY to $env:VCPKG_CONCURRENCY"
    }

    # Change to source directory so vcpkg can find vcpkg.json
    Push-Location $SourceDir
    try {
        & $vcpkgExe install --triplet x64-windows
        if ($LASTEXITCODE -ne 0) {
            Write-Error "vcpkg dependency installation failed"
            exit 1
        }
    } finally {
        Pop-Location
    }

    # Restore MSYS2 on PATH (needed for flex/bison during CMake configure)
    $env:PATH = $savedPath

    Write-Information "✓ vcpkg dependencies installed"
    Write-Output ""

    # Install Python dependencies using vcpkg Python
    # In manifest mode, vcpkg installs packages into vcpkg_installed/ in the working directory
    Write-Information "Installing Python dependencies..."
    $vcpkgPython = "$SourceDir\vcpkg_installed\x64-windows\tools\python3\python.exe"

    if (-not (Test-Path $vcpkgPython)) {
        # Fallback: check classic mode location
        $vcpkgPython = "$vcpkgDir\packages\python3_x64-windows\tools\python3\python.exe"
    }

    if (-not (Test-Path $vcpkgPython)) {
        Write-Error "vcpkg Python not found. Searched:`n  $SourceDir\vcpkg_installed\x64-windows\tools\python3\python.exe`n  $vcpkgDir\packages\python3_x64-windows\tools\python3\python.exe"
        Write-Information "       Make sure 'python3' is in vcpkg.json dependencies"
        exit 1
    }

    Write-Information "  Using Python: $vcpkgPython"

    # Bootstrap pip if not already installed
    Write-Information "  Ensuring pip is installed..."
    & $vcpkgPython -m ensurepip --default-pip
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to bootstrap pip"
        exit 1
    }

    & $vcpkgPython -m pip install --upgrade pip
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to upgrade pip"
        exit 1
    }

    & $vcpkgPython -m pip install bsdiff4 numpy pillow
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to install Python dependencies"
        exit 1
    }
    Write-Information "✓ Python dependencies installed"
    Write-Output ""
}

# Clean build directory if requested
if ($CleanBuild -and (Test-Path $BuildDir)) {
    Write-Information "Cleaning build directory..."
    Remove-Item -Recurse -Force $BuildDir
    Write-Information "✓ Build directory cleaned"
    Write-Output ""
}

# Configure and build with CMake
if (-not $SkipBuild) {
    Write-Information "Configuring CMake..."

    # Ensure VCPKG_ROOT is set (required by CMake preset toolchainFile)
    if (-not $env:VCPKG_ROOT) {
        if (Test-Path "C:\vcpkg\vcpkg.exe") {
            $env:VCPKG_ROOT = "C:\vcpkg"
        } elseif (Get-Command vcpkg -ErrorAction SilentlyContinue) {
            $env:VCPKG_ROOT = Split-Path (Get-Command vcpkg).Source
        } else {
            Write-Error "VCPKG_ROOT is not set and vcpkg was not found. Cannot configure CMake preset."
            exit 1
        }
        Write-Information "  VCPKG_ROOT set to: $env:VCPKG_ROOT"
    }

    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
    }

    # Locate flex, bison, and bash executables (from MSYS2) for explicit CMake variable passing
    # These must be resolved before MSYS2 is stripped from PATH
    $flexExe = (Get-Command flex -ErrorAction SilentlyContinue).Source
    $bisonExe = (Get-Command bison -ErrorAction SilentlyContinue).Source
    $bashExe = (Get-Command bash -ErrorAction SilentlyContinue).Source
    if (-not $flexExe) { $flexExe = "C:\msys64\usr\bin\flex.exe" }
    if (-not $bisonExe) { $bisonExe = "C:\msys64\usr\bin\bison.exe" }
    if (-not $bashExe) { $bashExe = "C:\msys64\usr\bin\bash.exe" }
    Write-Information "  Using flex: $flexExe"
    Write-Information "  Using bison: $bisonExe"
    Write-Information "  Using bash: $bashExe"

    # Remove MSYS2 from PATH during cmake configure to prevent sh.exe from
    # mangling Windows paths (C: -> C;) in vcpkg's internal CMake invocations
    $savedConfigPath = $env:PATH
    $env:PATH = ($env:PATH -split ';' | Where-Object { $_ -notmatch 'msys64|msys2|cygwin' }) -join ';'
    $env:MSYS_NO_PATHCONV = "1"
    $env:MSYS2_ARG_CONV_EXCL = "*"

    # First configuration: disable tests since Python isn't available yet
    Write-Information "  Initial configuration (tests disabled)..."
    & $cmakeExe --preset windows-msvc-release -DENABLE_TESTS=OFF `
        -DFLEX_EXECUTABLE="$flexExe" -DBISON_EXECUTABLE="$bisonExe" `
        -DSHELL_EXE="$bashExe"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        exit 1
    }

    # After first configuration, vcpkg will have installed Python
    # Set Python paths for CMake to find vcpkg Python for tests
    $vcpkgPythonTools = "$BuildDir\vcpkg_installed\x64-windows\tools\python3"
    Write-Information "  Checking for vcpkg Python at: $vcpkgPythonTools"
    
    if (Test-Path $vcpkgPythonTools) {
        Write-Information "  Found vcpkg Python directory"
        
        $pythonExe = "$vcpkgPythonTools\python.exe"
        if (Test-Path $pythonExe) {
            Write-Information "  Found python.exe at: $pythonExe"
            
            # Test Python executable
            $pythonVersion = & $pythonExe --version 2>&1
            Write-Information "  Python version: $pythonVersion"
            
            $env:Python3_ROOT_DIR = $vcpkgPythonTools
            $env:Python3_EXECUTABLE = $pythonExe
            $env:PATH = "$vcpkgPythonTools;$env:PATH"
            Write-Information "  Set Python3_ROOT_DIR=$vcpkgPythonTools"
            Write-Information "  Set Python3_EXECUTABLE=$pythonExe"
            
            # Reconfigure to enable tests and pick up Python
            Write-Information "  Reconfiguring with tests enabled and vcpkg Python..."
            & $cmakeExe --preset windows-msvc-release `
                -DENABLE_TESTS=ON `
                -DFLEX_EXECUTABLE="$flexExe" -DBISON_EXECUTABLE="$bisonExe" `
                -DSHELL_EXE="$bashExe" `
                -DPython3_ROOT_DIR="$vcpkgPythonTools" `
                -DPython3_EXECUTABLE="$pythonExe"
            if ($LASTEXITCODE -ne 0) {
                Write-Error "CMake reconfiguration failed"
                exit 1
            }
        } else {
            Write-Warning "  python.exe not found at: $pythonExe"
            Write-Information "  Contents of ${vcpkgPythonTools}:"
            Get-ChildItem $vcpkgPythonTools -ErrorAction SilentlyContinue | ForEach-Object { Write-Information "    - $($_.Name)" }
        }
    } else {
        Write-Warning "  vcpkg Python directory not found at: $vcpkgPythonTools"
        Write-Information "  Contents of vcpkg_installed:"
        Get-ChildItem "$BuildDir\vcpkg_installed" -ErrorAction SilentlyContinue | ForEach-Object { Write-Information "    - $($_.Name)" }
        if (Test-Path "$BuildDir\vcpkg_installed\x64-windows") {
            Write-Information "  Contents of vcpkg_installed\x64-windows:"
            Get-ChildItem "$BuildDir\vcpkg_installed\x64-windows" -ErrorAction SilentlyContinue | ForEach-Object { Write-Information "    - $($_.Name)" }
        }
        if (Test-Path "$BuildDir\vcpkg_installed\x64-windows\tools") {
            Write-Information "  Contents of vcpkg_installed\x64-windows\tools:"
            Get-ChildItem "$BuildDir\vcpkg_installed\x64-windows\tools" -ErrorAction SilentlyContinue | ForEach-Object { Write-Information "    - $($_.Name)" }
        }
    }

    Write-Information "✓ CMake configuration complete"
    Write-Output ""

    # Restore full PATH (MSYS2 back) for the build step
    $env:PATH = $savedConfigPath

    Write-Information "Building OpenSCAD..."
    & $cmakeExe --build --preset windows-msvc-release --verbose
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        exit 1
    }

    Write-Information "✓ Build complete"
    Write-Output ""

    # Copy Qt and dependency DLLs next to executable
    Write-Information "Copying runtime DLLs..."
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

    Write-Information "✓ Runtime DLLs copied"
    Write-Output ""
}

# Run tests if requested
if ($RunTests) {
    Write-Information "Running tests..."

    # Set Python environment variables for tests
    $pythonToolsDir = "$BuildDir\vcpkg_installed\x64-windows\tools\python3"
    if (Test-Path $pythonToolsDir) {
        $env:PYTHONHOME = $pythonToolsDir
        $env:PYTHONPATH = "$pythonToolsDir\Lib;$pythonToolsDir\Lib\site-packages"
    }

    # Add Qt and Mesa to PATH for tests
    $mesaInstallDir = "$env:LOCALAPPDATA\mesa"
    $env:PATH = "$Qt6Path\bin;$Qt6Path\lib;$mesaInstallDir\x64;$env:PATH"

    # Create a test runner that suppresses Windows error dialogs
    # This only affects test processes, not the entire system
    Write-Information "Configuring error handling to suppress crash dialog boxes..."
    
    # Use a simpler P/Invoke approach directly in PowerShell
    $setErrorModeSource = @"
using System;
using System.Runtime.InteropServices;

public class ErrorMode {
    [DllImport("kernel32.dll")]
    public static extern uint SetErrorMode(uint uMode);
    
    [DllImport("kernel32.dll")]
    public static extern uint GetErrorMode();
}
"@
    
    $errorModeLoaded = $false
    try {
        Add-Type -TypeDefinition $setErrorModeSource
        # Set error mode flags:
        # SEM_FAILCRITICALERRORS (0x0001) - No critical error handler
        # SEM_NOGPFAULTERRORBOX (0x0002) - No GP fault error box
        # SEM_NOOPENFILEERRORBOX (0x8000) - No open file error box
        $flags = 0x0001 -bor 0x0002 -bor 0x8000
        $oldMode = [ErrorMode]::SetErrorMode($flags)
        $errorModeLoaded = $true
        Write-Information "✓ Error mode set to 0x$($flags.ToString('X4')) (was 0x$($oldMode.ToString('X4')))"
        Write-Information "✓ Crash dialogs suppressed for test processes"
    } catch {
        Write-Warning "Could not set error mode, crash dialogs may appear: $_"
    }

    Push-Location $BuildDir
    try {
        $env:CTEST_OUTPUT_ON_FAILURE = 1
        
        # Verify error mode is still set
        if ($errorModeLoaded) {
            $currentMode = [ErrorMode]::GetErrorMode()
            Write-Information "Current error mode: 0x$($currentMode.ToString('X4'))"
        }

        $ctestArgs = "-C Release -L Default -j2 --output-on-failure"
        $ctestDescription = "full Default test suite"
        if ($RerunFailedTests) {
            $lastFailedLog = Join-Path $BuildDir "Testing/Temporary/LastTestsFailed.log"
            if ((Test-Path $lastFailedLog) -and ((Get-Content $lastFailedLog | Where-Object { $_.Trim().Length -gt 0 }).Count -gt 0)) {
                $ctestArgs = "-C Release --rerun-failed -j2 --output-on-failure"
                $ctestDescription = "previously failed tests"
                Write-Information "Detected LastTestsFailed.log. Rerunning only previously failed tests."
            } else {
                Write-Warning "No record of failed tests found. Running full suite instead."
            }
        }

        Write-Information "Invoking: ctest $ctestArgs ($ctestDescription)"
        Invoke-Expression "ctest $ctestArgs"
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Tests failed"
            exit 1
        }
        Write-Information "✓ Tests complete"
    } finally {
        Pop-Location
    }
    Write-Output ""
}

# Summary
Write-Information "========================================"
Write-Information "Build Summary"
Write-Information "========================================"
Write-Output "Source Directory: $SourceDir"
Write-Output "Build Directory:  $BuildDir"
Write-Output "Qt6 Path:         $Qt6Path"
Write-Output ""

if (Test-Path "$BuildDir\Release\openscad.exe") {
    Write-Information "✓ Executable: $BuildDir\Release\openscad.exe"
} else {
    Write-Warning " Executable not found (build may have failed)"
}

Write-Output ""
Write-Information "To run tests manually:"
Write-Information "  cd $BuildDir\tests"
Write-Information "  ctest -C Release -L Default --output-on-failure"
Write-Output ""
Write-Information "To run OpenSCAD:"
Write-Information "  $BuildDir\Release\openscad.exe"
Write-Output ""
