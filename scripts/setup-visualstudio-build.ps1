# PowerShell script to set up OpenSCAD build with Visual Studio and vcpkg
#
# Usage:
#   .\scripts\setup-visualstudio-build.ps1 [-BuildType Release|Debug|RelWithDebInfo] [-SkipVcpkg]
#
# Prerequisites:
#   - Visual Studio 2019 or 2022 with C++ workload
#   - CMake 3.21 or later
#   - Git

param(
    [Parameter()]
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo')]
    [string]$BuildType = 'Release',

    [Parameter()]
    [switch]$SkipVcpkg = $false,

    [Parameter()]
    [string]$VcpkgRoot = $env:VCPKG_ROOT
)

$ErrorActionPreference = "Stop"

Write-Host "=== OpenSCAD Visual Studio Build Setup ===" -ForegroundColor Cyan
Write-Host ""

# Check for Visual Studio
Write-Host "Checking for Visual Studio..." -ForegroundColor Yellow
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath
    if ($vsPath) {
        Write-Host "✓ Found Visual Studio at: $vsPath" -ForegroundColor Green
    } else {
        Write-Error "Visual Studio not found. Please install Visual Studio 2019 or 2022 with C++ workload."
        exit 1
    }
} else {
    Write-Error "vswhere.exe not found. Please install Visual Studio."
    exit 1
}

# Check for CMake
Write-Host "Checking for CMake..." -ForegroundColor Yellow
try {
    $cmakeVersion = cmake --version | Select-String -Pattern "cmake version (\d+\.\d+\.\d+)" | ForEach-Object { $_.Matches[0].Groups[1].Value }
    Write-Host "✓ Found CMake version: $cmakeVersion" -ForegroundColor Green

    $requiredVersion = [version]"3.21.0"
    $actualVersion = [version]$cmakeVersion
    if ($actualVersion -lt $requiredVersion) {
        Write-Error "CMake version 3.21 or later is required. Found: $cmakeVersion"
        exit 1
    }
} catch {
    Write-Error "CMake not found. Please install CMake 3.21 or later."
    exit 1
}

# Set up vcpkg if not skipped
if (-not $SkipVcpkg) {
    Write-Host ""
    Write-Host "Setting up vcpkg..." -ForegroundColor Yellow

    if (-not $VcpkgRoot) {
        # Check if vcpkg is in a default location
        $possiblePaths = @(
            "$PSScriptRoot\..\..\vcpkg",
            "$env:USERPROFILE\vcpkg",
            "C:\vcpkg"
        )

        foreach ($path in $possiblePaths) {
            if (Test-Path "$path\vcpkg.exe") {
                $VcpkgRoot = $path
                break
            }
        }

        if (-not $VcpkgRoot) {
            Write-Host "vcpkg not found. Cloning vcpkg repository..." -ForegroundColor Yellow
            $VcpkgRoot = "$PSScriptRoot\..\..\vcpkg"
            git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
            if ($LASTEXITCODE -ne 0) {
                Write-Error "Failed to clone vcpkg repository."
                exit 1
            }

            Write-Host "Bootstrapping vcpkg..." -ForegroundColor Yellow
            Push-Location $VcpkgRoot
            .\bootstrap-vcpkg.bat
            if ($LASTEXITCODE -ne 0) {
                Pop-Location
                Write-Error "Failed to bootstrap vcpkg."
                exit 1
            }
            Pop-Location
        }
    }

    Write-Host "✓ Using vcpkg at: $VcpkgRoot" -ForegroundColor Green
    $env:VCPKG_ROOT = $VcpkgRoot

    # Install dependencies using vcpkg.json manifest mode
    Write-Host ""
    Write-Host "Installing dependencies via vcpkg (this may take a while)..." -ForegroundColor Yellow
    Write-Host "Note: Dependencies are defined in vcpkg.json and will be installed automatically during CMake configure." -ForegroundColor Cyan
}

# Configure the build
Write-Host ""
Write-Host "Configuring OpenSCAD build..." -ForegroundColor Yellow
$presetName = "windows-msvc-" + $BuildType.ToLower()
Write-Host "Using CMake preset: $presetName" -ForegroundColor Cyan

cmake --preset $presetName
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed."
    exit 1
}

Write-Host ""
Write-Host "✓ Configuration complete!" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Build:   cmake --build --preset $presetName" -ForegroundColor White
Write-Host "  2. Test:    ctest --preset $presetName" -ForegroundColor White
Write-Host "  3. Install: cmake --install build/$presetName" -ForegroundColor White
Write-Host ""
Write-Host "Or open the solution in Visual Studio:" -ForegroundColor Cyan
Write-Host "  Visual Studio → Open → CMake → select CMakeLists.txt" -ForegroundColor White
Write-Host "  Then select the '$presetName' configuration" -ForegroundColor White
Write-Host ""
