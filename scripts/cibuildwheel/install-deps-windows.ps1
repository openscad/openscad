# Install native build dependencies for Windows wheel builds (MSVC ABI).
# Runs as cibuildwheel [tool.cibuildwheel.windows] before-all.
$ErrorActionPreference = "Stop"

Write-Host "=== install-deps-windows.ps1: bootstrapping vcpkg ==="

function Assert-NativeCommandSucceeded {
    param([string]$Description)
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed with exit code $LASTEXITCODE"
    }
}

function Get-ExistingPath {
    param([string[]]$Candidates)
    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            return $Candidate
        }
    }
    return $null
}

function Remove-Msys2PathEntries {
    param([string]$PathValue)
    $MsysRoot = "C:\msys64\"
    $Separator = [IO.Path]::PathSeparator
    (($PathValue -split [Regex]::Escape($Separator)) | Where-Object {
        $Normalized = $_.Replace("/", "\")
        $_ -and -not $Normalized.StartsWith($MsysRoot, [StringComparison]::OrdinalIgnoreCase)
    }) -join $Separator
}

$ProjectRoot = if ($env:CIBW_PROJECT_DIR) {
    $env:CIBW_PROJECT_DIR
} elseif ($env:GITHUB_WORKSPACE) {
    $env:GITHUB_WORKSPACE
} else {
    (Get-Location).Path
}

# Match the native MSVC workflow: vcpkg supplies libraries, MSYS2 supplies
# parser generators. GitHub's Windows runners already include C:\msys64.
$MsysUsrBin = Get-ExistingPath @("C:\msys64\usr\bin", "C:\msys64\ucrt64\bin")
if (-not $MsysUsrBin) {
    throw "MSYS2 was not found under C:\msys64; install MSYS2 or run msys2/setup-msys2 before cibuildwheel"
}

$Pacman = Join-Path $MsysUsrBin "pacman.exe"
if (Test-Path $Pacman) {
    & $Pacman -Sy --noconfirm flex bison
    Assert-NativeCommandSucceeded "pacman install flex bison"
}

$BisonExe = Get-ExistingPath @(
    (Join-Path $MsysUsrBin "bison.exe"),
    (Join-Path $MsysUsrBin "win_bison.exe")
)
$FlexExe = Get-ExistingPath @(
    (Join-Path $MsysUsrBin "flex.exe"),
    (Join-Path $MsysUsrBin "win_flex.exe")
)
if (-not $BisonExe -or -not $FlexExe) {
    throw "MSYS2 flex/bison tools were not found after install (bison: $BisonExe, flex: $FlexExe)"
}

# Pin vcpkg to a release tag for reproducible builds (see vcpkg.json builtin-baseline).
$VcpkgVersion = "2026.06.24"
$VcpkgRoot = Join-Path $ProjectRoot ".wheel-vcpkg"
$ManifestDir = Join-Path $ProjectRoot "scripts/cibuildwheel"
$VcpkgManifest = Join-Path $ManifestDir "vcpkg.json"

if (-not (Test-Path $VcpkgRoot)) {
    # vcpkg versioned ports need historical tree objects; shallow clones fail during install.
    git clone --branch $VcpkgVersion `
        https://github.com/microsoft/vcpkg.git $VcpkgRoot
    Assert-NativeCommandSucceeded "git clone vcpkg"
} else {
    Push-Location $VcpkgRoot
    try {
        git fetch origin "refs/tags/${VcpkgVersion}:refs/tags/${VcpkgVersion}"
        Assert-NativeCommandSucceeded "git fetch vcpkg tag"
        $IsShallow = (git rev-parse --is-shallow-repository).Trim()
        Assert-NativeCommandSucceeded "git check vcpkg shallow state"
        if ($IsShallow -eq "true") {
            git fetch --unshallow origin
            Assert-NativeCommandSucceeded "git unshallow vcpkg"
        }
        git checkout $VcpkgVersion
        Assert-NativeCommandSucceeded "git checkout vcpkg tag"
    } finally {
        Pop-Location
    }
}

$ManifestBaseline = ((Get-Content $VcpkgManifest -Raw) | ConvertFrom-Json)."builtin-baseline"
Push-Location $VcpkgRoot
try {
    $CheckedOutCommit = (git rev-parse HEAD).Trim()
    Assert-NativeCommandSucceeded "git rev-parse vcpkg HEAD"
} finally {
    Pop-Location
}
if ($ManifestBaseline -ne $CheckedOutCommit) {
    throw "vcpkg tag $VcpkgVersion resolves to $CheckedOutCommit, but vcpkg.json builtin-baseline is $ManifestBaseline. Update both pins together."
}

$VcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $VcpkgExe)) {
    & (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics
    Assert-NativeCommandSucceeded "bootstrap vcpkg"
}

# Use vcpkg's pinned community release triplet; keep this in sync with pyproject.toml.
$Triplet = "x64-windows-release"
$HostTriplet = "x64-windows-release"
$Installed = Join-Path $VcpkgRoot "installed" $Triplet

Push-Location $ManifestDir
try {
    $OriginalPath = $env:PATH
    $env:PATH = Remove-Msys2PathEntries $env:PATH
    & $VcpkgExe install `
        --triplet $Triplet `
        --host-triplet $HostTriplet `
        --x-manifest-root $ManifestDir `
        --x-install-root (Join-Path $VcpkgRoot "installed")
    Assert-NativeCommandSucceeded "vcpkg install"
} finally {
    if ($null -ne $OriginalPath) {
        $env:PATH = $OriginalPath
    }
    Pop-Location
}

$PkgConfigDir = Join-Path $Installed "lib" "pkgconfig"
$PkgConfExe = Join-Path $Installed "tools" "pkgconf" "pkgconf.exe"

# before-all runs in a separate process; persist env for setup.py and repair-wheel.
$EnvFile = Join-Path $ProjectRoot "scripts/cibuildwheel/wheel-build-env.env"
$EnvLines = @(
    "VCPKG_ROOT=$VcpkgRoot",
    "VCPKG_DEFAULT_TRIPLET=$Triplet",
    "VCPKG_DEFAULT_HOST_TRIPLET=$HostTriplet",
    "PKG_CONFIG_PATH=$PkgConfigDir",
    "PKG_CONFIG=$PkgConfExe",
    "BISON=$BisonExe",
    "FLEX=$FlexExe",
    "MSYS2_USR_BIN=$MsysUsrBin"
)
$Utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllLines($EnvFile, $EnvLines, $Utf8NoBom)

Write-Host "=== install-deps-windows.ps1: done (vcpkg root: $VcpkgRoot) ==="
