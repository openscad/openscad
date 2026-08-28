# Run OpenSCAD Tests with Proper Environment
# This script sets up the environment and runs ctest

param(
    [string]$BuildDir = "$PSScriptRoot\..\build\windows-msvc-release",
    [string]$Qt6Path = "C:\Qt\6.10.0\msvc2022_64",
    [string]$TestFilter = "",  # Optional: filter tests by regex
    [int]$Jobs = $env:NUMBER_OF_PROCESSORS,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

# First, ensure environment is set up
& "$PSScriptRoot\setup-test-environment.ps1" -BuildDir $BuildDir -Qt6Path $Qt6Path

Write-Host "Running tests..." -ForegroundColor Cyan
Write-Host ""

# Set environment variables for this process
$VcpkgDir = "$BuildDir\vcpkg_installed\x64-windows"
$env:FONTCONFIG_FILE = "$VcpkgDir\etc\fonts\fonts.conf"
$env:FONTCONFIG_PATH = "$VcpkgDir\etc\fonts"
$env:QT_OPENGL = "software"
$env:QT_QPA_PLATFORM = "offscreen"

# Add Qt to PATH
$env:PATH = "$Qt6Path\bin;$Qt6Path\lib;$env:PATH"

# Verify environment is set
Write-Host "Environment variables:" -ForegroundColor Gray
Write-Host "  FONTCONFIG_FILE = $env:FONTCONFIG_FILE" -ForegroundColor Gray
Write-Host "  FONTCONFIG_PATH = $env:FONTCONFIG_PATH" -ForegroundColor Gray
Write-Host "  QT_OPENGL = $env:QT_OPENGL" -ForegroundColor Gray
Write-Host "  QT_QPA_PLATFORM = $env:QT_QPA_PLATFORM" -ForegroundColor Gray
Write-Host ""

# Change to build directory
Push-Location $BuildDir

try {
    # Build ctest command
    $ctestArgs = @("-C", "Release", "-L", "Default", "--output-on-failure", "-j", $Jobs)

    if ($Verbose) {
        $ctestArgs += "-V"
    }

    if ($TestFilter) {
        $ctestArgs += @("-R", $TestFilter)
    }

    Write-Host "Running: ctest $($ctestArgs -join ' ')" -ForegroundColor Gray
    Write-Host ""

    # Environment variables are already set above and will be inherited by ctest and its child processes
    & ctest @ctestArgs

    $exitCode = $LASTEXITCODE
    Write-Host ""

    if ($exitCode -eq 0) {
        Write-Host "✓ All tests passed!" -ForegroundColor Green
    } else {
        Write-Host "⚠ Some tests failed (exit code: $exitCode)" -ForegroundColor Yellow
    }

    exit $exitCode
} finally {
    Pop-Location
}
