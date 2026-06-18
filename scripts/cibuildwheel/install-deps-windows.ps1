# Install native build dependencies for Windows wheel builds (MSVC ABI).
# Runs as cibuildwheel [tool.cibuildwheel.windows] before-all.
$ErrorActionPreference = "Stop"

Write-Host "=== install-deps-windows.ps1: bootstrapping vcpkg ==="

$ProjectRoot = if ($env:CIBW_PROJECT_DIR) {
    $env:CIBW_PROJECT_DIR
} elseif ($env:GITHUB_WORKSPACE) {
    $env:GITHUB_WORKSPACE
} else {
    (Get-Location).Path
}

# Pin vcpkg to a release tag for reproducible builds (see vcpkg.json builtin-baseline).
$VcpkgVersion = "2025.04.09"
$VcpkgRoot = Join-Path $ProjectRoot ".wheel-vcpkg"

if (-not (Test-Path $VcpkgRoot)) {
    git clone --depth 1 --branch $VcpkgVersion `
        https://github.com/microsoft/vcpkg.git $VcpkgRoot
} else {
    Push-Location $VcpkgRoot
    try {
        git fetch --depth 1 origin "refs/tags/${VcpkgVersion}:refs/tags/${VcpkgVersion}"
        git checkout $VcpkgVersion
    } finally {
        Pop-Location
    }
}

$VcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $VcpkgExe)) {
    & (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics
}

$ManifestDir = Join-Path $ProjectRoot "scripts/cibuildwheel"
$Triplet = "x64-windows"

Push-Location $ManifestDir
try {
    & $VcpkgExe install `
        --triplet $Triplet `
        --x-manifest-root $ManifestDir `
        --x-install-root (Join-Path $VcpkgRoot "installed")
} finally {
    Pop-Location
}

$Installed = Join-Path $VcpkgRoot "installed" $Triplet
$PkgConfigDir = Join-Path $Installed "lib" "pkgconfig"
$PkgConfExe = Join-Path $Installed "tools" "pkgconf" "pkgconf.exe"

# winflexbison installs win_bison.exe / win_flex.exe under tools/winflexbison/.
$WinFlexDir = Join-Path $Installed "tools" "winflexbison"

# before-all runs in a separate process; persist env for setup.py and repair-wheel.
$EnvFile = Join-Path $ProjectRoot "scripts/cibuildwheel/wheel-build-env.env"
$EnvLines = @(
    "VCPKG_ROOT=$VcpkgRoot",
    "VCPKG_DEFAULT_TRIPLET=$Triplet",
    "PKG_CONFIG_PATH=$PkgConfigDir",
    "PKG_CONFIG=$PkgConfExe"
)
if (Test-Path $WinFlexDir) {
    $EnvLines += "WINFLEXBISON_DIR=$WinFlexDir"
}
$EnvLines | Set-Content -Encoding utf8 $EnvFile

Write-Host "=== install-deps-windows.ps1: done (vcpkg root: $VcpkgRoot) ==="
