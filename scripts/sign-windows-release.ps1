<#
.SYNOPSIS
    Sign PythonSCAD Windows release artifacts using Certum SimplySign.

.DESCRIPTION
    Downloads the NSIS installer (.exe) and, if present, the MSIX package from
    a GitHub pre-release, signs them with signtool via the SimplySign cloud HSM,
    verifies the signatures, re-uploads the signed files, and then triggers the
    Publish Release workflow to compute checksums and make the release public.

    Prerequisites (must be done before running this script):
      1. SimplySign Desktop is running and connected (right-click tray icon ->
         "Connect to SimplySign", enter your username + the token from the
         SimplySign mobile app).
      2. Windows SDK signtool.exe is installed (comes with Visual Studio or the
         standalone Windows SDK).
      3. GitHub CLI (gh) is installed and authenticated.

.PARAMETER Tag
    The release tag to sign (e.g. "v0.20.0"). If omitted the script lists
    recent pre-releases and asks you to choose.

.PARAMETER Thumbprint
    SHA1 thumbprint of your Certum code-signing certificate.
    Default: 4423B0926E55728D678DE86C3B7F2FC1B1C76A59

.PARAMETER TimestampUrl
    RFC 3161 timestamp server URL.
    Default: http://time.certum.pl

.PARAMETER Repo
    GitHub repository in "owner/name" form used for gh CLI calls.
    Default: pythonscad/pythonscad
    Override this when working with a fork or a mirror.

.PARAMETER WorkDir
    Temporary directory for downloaded artifacts.
    Default: $env:TEMP\pythonscad-sign

.EXAMPLE
    # Sign the latest pre-release (script will prompt if multiple exist)
    .\scripts\sign-windows-release.ps1

.EXAMPLE
    # Sign a specific release tag
    .\scripts\sign-windows-release.ps1 -Tag v0.20.0

.EXAMPLE
    # Sign against a fork
    .\scripts\sign-windows-release.ps1 -Tag v0.20.0 -Repo myfork/pythonscad
#>

[CmdletBinding()]
param(
    [string]$Tag          = '',
    [string]$Thumbprint   = '4423B0926E55728D678DE86C3B7F2FC1B1C76A59',
    [string]$TimestampUrl = 'http://time.certum.pl',
    [string]$Repo         = 'pythonscad/pythonscad',
    [string]$WorkDir      = (Join-Path $env:TEMP 'pythonscad-sign')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
function Write-Step([string]$Msg) { Write-Host "`n==> $Msg" -ForegroundColor Cyan }
function Write-OK([string]$Msg)   { Write-Host "    $Msg" -ForegroundColor Green }
function Write-Warn([string]$Msg) { Write-Host "    WARNING: $Msg" -ForegroundColor Yellow }

# ---------------------------------------------------------------------------
# 1. Locate signtool
# ---------------------------------------------------------------------------
Write-Step "Locating signtool.exe"
$signtool = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe" `
    -ErrorAction SilentlyContinue |
    Sort-Object { [version]($_.Directory.Parent.Name) } -Descending |
    Select-Object -First 1 -ExpandProperty FullName

if (-not $signtool) {
    throw "signtool.exe not found. Install the Windows SDK or Visual Studio."
}
Write-OK "Found: $signtool"

# ---------------------------------------------------------------------------
# 2. Confirm SimplySign certificate is visible in the Windows store
# ---------------------------------------------------------------------------
Write-Step "Checking for Certum certificate in Windows store (Thumbprint: $Thumbprint)"
$cert = Get-ChildItem Cert:\CurrentUser\My |
    Where-Object { $_.Thumbprint -eq $Thumbprint.ToUpper() }

if (-not $cert) {
    Write-Host @"

    Certificate NOT found. SimplySign Desktop must be running and connected.

    Steps:
      1. Right-click the SimplySign Desktop tray icon (near the clock)
      2. Choose "Connect to SimplySign"
      3. Open the SimplySign mobile app and generate a token
      4. Enter your username + token in the dialog, click OK
      5. Re-run this script

"@ -ForegroundColor Red
    exit 1
}
Write-OK "Found: $($cert.Subject)"
Write-OK "Valid until: $($cert.NotAfter)"
$daysUntilExpiry = ($cert.NotAfter - (Get-Date)).Days
if ($daysUntilExpiry -le 30) {
    Write-Warn "Certificate expires in $daysUntilExpiry day(s) ($($cert.NotAfter.ToString('yyyy-MM-dd'))). Renew soon!"
}

# ---------------------------------------------------------------------------
# 3. Resolve release tag
# ---------------------------------------------------------------------------
Write-Step "Resolving release tag"

if (-not $Tag) {
    Write-Host "    No tag specified, fetching recent pre-releases from $Repo ..."
    $json = gh release list --repo $Repo --json tagName,name,isDraft,isPrerelease --limit 10
    if ($LASTEXITCODE -ne 0) { throw "gh release list failed (exit $LASTEXITCODE)" }
    $releases = $json | ConvertFrom-Json | Where-Object { $_.isPrerelease -or $_.isDraft }
    if (-not $releases) {
        throw "No pre-release or draft releases found in $Repo. Use -Tag to specify one explicitly."
    }
    if ($releases.Count -eq 1) {
        $Tag = $releases[0].tagName
        Write-OK "Auto-selected: $Tag ($($releases[0].name))"
    } else {
        Write-Host "`n    Available pre-releases / drafts:" -ForegroundColor Yellow
        for ($i = 0; $i -lt $releases.Count; $i++) {
            Write-Host "      [$i] $($releases[$i].tagName)  -  $($releases[$i].name)"
        }
        $choice = Read-Host "    Enter number"
        $idx = $null
        if (-not [int]::TryParse($choice, [ref]$idx) -or $idx -lt 0 -or $idx -ge $releases.Count) {
            throw "Invalid choice '$choice'. Enter a number between 0 and $($releases.Count - 1)."
        }
        $Tag = $releases[$idx].tagName
        Write-OK "Selected: $Tag"
    }
} else {
    Write-OK "Tag: $Tag"
}

# ---------------------------------------------------------------------------
# 4. Download signable Windows artifacts
# ---------------------------------------------------------------------------
Write-Step "Downloading Windows artifacts for $Tag from $Repo"

if (Test-Path $WorkDir) { Remove-Item $WorkDir -Recurse -Force }
New-Item -ItemType Directory -Path $WorkDir | Out-Null

# Download the NSIS installer and (if present) the MSIX package.
# --pattern is OR-ed; gh silently skips patterns that match nothing, so a
# release without an MSIX is not an error.
gh release download $Tag --repo $Repo --dir $WorkDir `
    --pattern "*-Installer.exe" `
    --pattern "*.msix"

$artifacts = Get-ChildItem $WorkDir -File | Where-Object { $_.Extension -in '.exe', '.msix' }
if (-not $artifacts) {
    throw "No signable artifacts (.exe or .msix) found in release $Tag."
}

Write-OK "Artifacts to sign:"
$artifacts | ForEach-Object { Write-OK "  $($_.Name)  ($([math]::Round($_.Length/1MB, 1)) MB)" }

# ---------------------------------------------------------------------------
# 5. Sign each artifact
# ---------------------------------------------------------------------------
Write-Step "Signing artifacts with Certum SimplySign"
Write-Host "    This certificate is pinless — signing proceeds immediately." -ForegroundColor Yellow

$failed = @()
foreach ($file in $artifacts) {
    Write-Host "`n    Signing: $($file.Name)" -ForegroundColor White
    & $signtool sign `
        /sha1  $Thumbprint `
        /tr    $TimestampUrl `
        /td    sha256 `
        /fd    sha256 `
        /v     $file.FullName

    if ($LASTEXITCODE -ne 0) {
        Write-Warn "signtool exited $LASTEXITCODE for $($file.Name)"
        $failed += $file.Name
    }
}

if ($failed.Count -gt 0) {
    throw "Signing failed for: $($failed -join ', ')"
}
Write-OK "All artifacts signed."

# ---------------------------------------------------------------------------
# 6. Verify signatures
# ---------------------------------------------------------------------------
Write-Step "Verifying signatures"

$verifyFailed = @()
foreach ($file in $artifacts) {
    $sig = Get-AuthenticodeSignature $file.FullName
    if ($sig.Status -eq 'Valid') {
        Write-OK "$($file.Name): Valid  (signer: $($sig.SignerCertificate.Subject.Split(',')[0]))"
    } else {
        Write-Warn "$($file.Name): $($sig.Status) - $($sig.StatusMessage)"
        $verifyFailed += $file.Name
    }
}

if ($verifyFailed.Count -gt 0) {
    throw "Signature verification failed for: $($verifyFailed -join ', ')"
}

# ---------------------------------------------------------------------------
# 7. Re-upload signed artifacts
# ---------------------------------------------------------------------------
Write-Step "Re-uploading signed artifacts to release $Tag"

foreach ($file in $artifacts) {
    Write-Host "    Uploading: $($file.Name)"
    gh release upload $Tag $file.FullName --repo $Repo --clobber
}

Write-OK "Upload complete."

# ---------------------------------------------------------------------------
# 8. Trigger the Publish Release workflow
#    This computes checksums for all artifacts (including the newly signed
#    ones) and removes the pre-release flag.
# ---------------------------------------------------------------------------
Write-Step "Triggering Publish Release workflow on $Repo"

gh workflow run publish-release.yml --repo $Repo --field release_tag=$Tag

Write-Host @"

==> All done!

    The Publish Release workflow has been triggered on $Repo.
    It will compute checksums for all release artifacts (including the
    signed files), upload them, and make the release public.

    Monitor progress at:
    https://github.com/$Repo/actions/workflows/publish-release.yml

"@ -ForegroundColor Green

Remove-Item $WorkDir -Recurse -Force -ErrorAction SilentlyContinue
