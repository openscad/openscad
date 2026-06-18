# Repair a Windows wheel by bundling extension DLL dependencies.
param(
    [Parameter(Mandatory = $true)]
    [string]$Wheel,

    [Parameter(Mandatory = $true)]
    [string]$DestDir
)

$ErrorActionPreference = "Stop"

function Import-WheelBuildEnv {
    $ProjectRoot = if ($env:CIBW_PROJECT_DIR) {
        $env:CIBW_PROJECT_DIR
    } elseif ($env:GITHUB_WORKSPACE) {
        $env:GITHUB_WORKSPACE
    } else {
        (Get-Location).Path
    }
    $EnvFile = Join-Path $ProjectRoot "scripts/cibuildwheel/wheel-build-env.env"
    if (-not (Test-Path $EnvFile)) {
        throw "wheel-build-env.env not found at $EnvFile (run install-deps-windows.ps1 first)"
    }
    Get-Content $EnvFile | ForEach-Object {
        if ($_ -match '^\s*#' -or $_ -match '^\s*$') { return }
        $parts = $_ -split '=', 2
        if ($parts.Count -eq 2) {
            Set-Item -Path "env:$($parts[0].Trim())" -Value $parts[1].Trim()
        }
    }
}

Import-WheelBuildEnv

if (-not $env:VCPKG_ROOT) {
    throw "VCPKG_ROOT is not set after loading wheel-build-env.env"
}
if (-not $env:VCPKG_DEFAULT_TRIPLET) {
    throw "VCPKG_DEFAULT_TRIPLET is not set after loading wheel-build-env.env"
}

New-Item -ItemType Directory -Force -Path $DestDir | Out-Null

$VcpkgBin = Join-Path $env:VCPKG_ROOT "installed" $env:VCPKG_DEFAULT_TRIPLET "bin"

delvewheel repair `
    --add-path $VcpkgBin `
    --exclude "python*.dll;vcruntime*.dll;api-ms-win*.dll;ucrtbase.dll" `
    -w $DestDir `
    $Wheel
