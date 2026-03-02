# Setup Test Environment for OpenSCAD
# This script prepares the environment for running OpenSCAD tests on Windows

param(
    [string]$BuildDir = "$PSScriptRoot\..\build\windows-msvc-release",
    [string]$Qt6Path = "C:\Qt\6.10.0\msvc2022_64"
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Setting up OpenSCAD Test Environment" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$ReleaseDir = "$BuildDir\Release"
$VcpkgDir = "$BuildDir\vcpkg_installed\x64-windows"

# 1. Copy Qt platform plugins
Write-Host "Copying Qt platform plugins..." -ForegroundColor Yellow
$QtPluginsDir = "$Qt6Path\plugins"

if (Test-Path "$QtPluginsDir\platforms") {
    New-Item -ItemType Directory -Path "$ReleaseDir\platforms" -Force | Out-Null
    Copy-Item "$QtPluginsDir\platforms\*.dll" -Destination "$ReleaseDir\platforms\" -Force
    Write-Host "  ✓ Copied platforms plugins" -ForegroundColor Green
}

if (Test-Path "$QtPluginsDir\styles") {
    New-Item -ItemType Directory -Path "$ReleaseDir\styles" -Force | Out-Null
    Copy-Item "$QtPluginsDir\styles\*.dll" -Destination "$ReleaseDir\styles\" -Force
    Write-Host "  ✓ Copied styles plugins" -ForegroundColor Green
}

if (Test-Path "$QtPluginsDir\imageformats") {
    New-Item -ItemType Directory -Path "$ReleaseDir\imageformats" -Force | Out-Null
    Copy-Item "$QtPluginsDir\imageformats\*.dll" -Destination "$ReleaseDir\imageformats\" -Force
    Write-Host "  ✓ Copied imageformats plugins" -ForegroundColor Green
}

Write-Host ""

# 2. Set up fontconfig environment variables
Write-Host "Setting up fontconfig..." -ForegroundColor Yellow
$FontconfigFile = "$VcpkgDir\etc\fonts\fonts.conf"
$FontconfigDir = "$VcpkgDir\etc\fonts"

if (Test-Path $FontconfigFile) {
    $env:FONTCONFIG_FILE = $FontconfigFile
    $env:FONTCONFIG_PATH = $FontconfigDir
    Write-Host "  ✓ FONTCONFIG_FILE = $FontconfigFile" -ForegroundColor Green
    Write-Host "  ✓ FONTCONFIG_PATH = $FontconfigDir" -ForegroundColor Green
} else {
    Write-Host "  ⚠ Warning: fontconfig files not found at $FontconfigFile" -ForegroundColor Yellow
}

Write-Host ""

# 3. Set up Qt environment for software OpenGL rendering
Write-Host "Setting up Qt OpenGL environment..." -ForegroundColor Yellow
$env:QT_OPENGL = "software"
$env:QT_QPA_PLATFORM = "offscreen"
Write-Host "  ✓ QT_OPENGL = software" -ForegroundColor Green
Write-Host "  ✓ QT_QPA_PLATFORM = offscreen" -ForegroundColor Green

Write-Host ""

# 4. Verify Mesa OpenGL is present
Write-Host "Verifying Mesa OpenGL..." -ForegroundColor Yellow
if (Test-Path "$ReleaseDir\opengl32sw.dll") {
    Write-Host "  ✓ Mesa OpenGL (opengl32sw.dll) found" -ForegroundColor Green
} else {
    Write-Host "  ⚠ Warning: opengl32sw.dll not found" -ForegroundColor Yellow
    Write-Host "  Mesa software renderer may not be available" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Environment Setup Complete" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "You can now run tests with:" -ForegroundColor Yellow
Write-Host "  cd $BuildDir" -ForegroundColor Gray
Write-Host "  ctest -C Release -L Default --output-on-failure -j `$env:NUMBER_OF_PROCESSORS" -ForegroundColor Gray
Write-Host ""
