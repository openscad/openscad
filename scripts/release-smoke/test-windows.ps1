$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $ScriptDir 'common-smoke.ps1')

function Show-Usage {
    @'
Usage: powershell -ExecutionPolicy Bypass -File test-windows.ps1 [options]

Download and smoke test Windows ZIP and NSIS installer artifacts.
MSIX artifacts are intentionally ignored for now.

Options:
  --repo <owner/name>          GitHub repository to query. Defaults to current repo.
  --ref <branch-or-tag>        Branch used for latest-run filtering.
  --run-id <id>                Use a specific build-windows-native.yml run.
  --workdir <path>             Working directory. Defaults to a temp directory.
  --keep-workdir               Keep temporary files after the run.
  --skip-download              Use artifacts already present in the artifact dir.
  --artifact-dir <path>        Directory containing ZIP/NSIS deployables.
  --skip-nsis-install          Skip uninstall/install testing of the NSIS installer.
  --skip-uninstall-existing    Do not uninstall an existing PythonSCAD first.
  --help                       Show this help.
'@
}

function Get-DefaultRepo {
    (& gh repo view --json nameWithOwner --jq .nameWithOwner).Trim()
}

function Get-LatestSuccessfulRun {
    param(
        [Parameter(Mandatory = $true)][string]$Repo,
        [Parameter(Mandatory = $true)][string]$Workflow,
        [string]$Ref
    )

    $ghArgs = @(
        'run', 'list',
        '--repo', $Repo,
        '--workflow', $Workflow,
        '--status', 'success',
        '--json', 'databaseId,headBranch'
    )
    if ($Ref) {
        if ($Ref -match '["\\\r\n]') {
            throw '--ref contains characters that cannot be used in GitHub Actions jq filters'
        }
        $jq = 'map(select(.headBranch == "' + $Ref + '")) | .[0].databaseId // ""'
        $ghArgs += @('--limit', '50', '--jq', $jq)
    } else {
        $ghArgs += @('--limit', '1', '--jq', '.[0].databaseId')
    }
    $runId = (& gh @ghArgs).Trim()
    if (-not $runId -or $runId -eq 'null') {
        throw "No successful $Workflow run found"
    }
    $runId
}

function Get-SmokeOption {
    param([string[]]$Arguments)

    $options = [ordered]@{
        Repo = ''
        Ref = ''
        RunId = ''
        Workdir = ''
        KeepWorkdir = $false
        SkipDownload = $false
        ArtifactDir = ''
        SkipNsisInstall = $false
        SkipUninstallExisting = $false
        Help = $false
    }

    for ($i = 0; $i -lt $Arguments.Count; $i++) {
        $arg = $Arguments[$i]
        switch ($arg) {
            '--repo' {
                $i++
                if ($i -ge $Arguments.Count) { throw '--repo requires a value' }
                $options.Repo = $Arguments[$i]
            }
            '--ref' {
                $i++
                if ($i -ge $Arguments.Count) { throw '--ref requires a value' }
                $options.Ref = $Arguments[$i]
            }
            '--run-id' {
                $i++
                if ($i -ge $Arguments.Count) { throw '--run-id requires a value' }
                $options.RunId = $Arguments[$i]
            }
            '--workdir' {
                $i++
                if ($i -ge $Arguments.Count) { throw '--workdir requires a value' }
                $options.Workdir = $Arguments[$i]
            }
            '--keep-workdir' {
                $options.KeepWorkdir = $true
            }
            '--skip-download' {
                $options.SkipDownload = $true
            }
            '--artifact-dir' {
                $i++
                if ($i -ge $Arguments.Count) { throw '--artifact-dir requires a value' }
                $options.ArtifactDir = $Arguments[$i]
            }
            '--skip-nsis-install' {
                $options.SkipNsisInstall = $true
            }
            '--skip-uninstall-existing' {
                $options.SkipUninstallExisting = $true
            }
            { $_ -in @('--help', '-h') } {
                $options.Help = $true
            }
            default {
                throw "Unknown option: $arg"
            }
        }
    }

    [pscustomobject]$options
}

function Find-PythonSCADExecutable {
    param([Parameter(Mandatory = $true)][string]$Root)

    $consoleExe = Get-ChildItem -LiteralPath $Root -Recurse -File -Filter 'pythonscad.com' -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($consoleExe) {
        return $consoleExe.FullName
    }

    $guiExe = Get-ChildItem -LiteralPath $Root -Recurse -File -Filter 'pythonscad.exe' -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($guiExe) {
        return $guiExe.FullName
    }

    throw "Could not locate pythonscad executable under $Root"
}

function Split-CommandArgument {
    param([string]$Arguments)

    if (-not $Arguments) {
        return @()
    }

    $tokens = @()
    foreach ($match in [regex]::Matches($Arguments, '(?:"([^"]*)"|(\S+))')) {
        if ($match.Groups[1].Success) {
            $tokens += $match.Groups[1].Value
        } else {
            $tokens += $match.Groups[2].Value
        }
    }
    return $tokens
}

function Invoke-ExistingPythonSCADUninstaller {
    $registryRoots = @(
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
    )

    foreach ($root in $registryRoots) {
        if (-not (Test-Path -LiteralPath $root)) {
            continue
        }
        $entries = Get-ChildItem -LiteralPath $root |
            Get-ItemProperty |
            Where-Object { $_.DisplayName -like '*PythonSCAD*' -and $_.UninstallString }
        foreach ($entry in $entries) {
            Write-SmokeLog "Uninstalling existing $($entry.DisplayName)"
            $uninstall = $entry.UninstallString.Trim()
            if ($uninstall -match '^\s*"(?<exe>[^"]+)"\s*(?<args>.*)$' -or
                $uninstall -match '^\s*(?<exe>\S+?\.exe)\s*(?<args>.*)$') {
                $exe = $Matches.exe
                $argumentList = Split-CommandArgument -Arguments $Matches.args
                $argumentList += '/S'
                Start-Process -FilePath $exe -ArgumentList $argumentList -Wait
            } else {
                Write-SmokeWarning "Could not parse uninstall command: $uninstall"
            }
        }
    }
}

$options = Get-SmokeOption -Arguments $args
if ($options.Help) {
    Show-Usage
    exit 0
}

$workdir = $options.Workdir
if (-not $workdir) {
    $workdir = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
}
New-Item -ItemType Directory -Force -Path $workdir | Out-Null

$scriptFailed = $false
try {
    $artifactDir = $options.ArtifactDir
    if (-not $artifactDir) {
        $artifactDir = Join-Path $workdir 'artifacts'
    }
    New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null

    if (-not $options.SkipDownload) {
        Assert-Command -Name gh
        $repo = $options.Repo
        if (-not $repo) {
            $repo = Get-DefaultRepo
        }

        $runId = $options.RunId
        if (-not $runId) {
            $runId = Get-LatestSuccessfulRun -Repo $repo -Workflow 'build-windows-native.yml' -Ref $options.Ref
        }
        Write-SmokeLog "Downloading Windows artifacts from run $runId"
        & gh run download $runId --repo $repo --dir $artifactDir
    } else {
        Write-SmokeLog "Using existing Windows artifacts in $artifactDir"
    }

    $zip = Get-ChildItem -LiteralPath $artifactDir -Recurse -File -Filter 'PythonSCAD-*.zip' |
        Where-Object { $_.Name -notlike '*Installer*' } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $zip) {
        throw "No ZIP deployable found in $artifactDir"
    }

    $zipExtractDir = Join-Path $workdir 'zip-extract'
    New-Item -ItemType Directory -Force -Path $zipExtractDir | Out-Null
    Write-SmokeLog "Extracting ZIP $($zip.Name)"
    Expand-Archive -LiteralPath $zip.FullName -DestinationPath $zipExtractDir -Force
    $zipExecutable = Find-PythonSCADExecutable -Root $zipExtractDir
    Invoke-SmokeTest -ExecutablePath $zipExecutable -Label "ZIP $($zip.Name)" -Workdir $workdir

    if ($options.SkipNsisInstall) {
        Write-SmokeLog 'Skipping NSIS installer smoke test'
    } else {
        $installer = Get-ChildItem -LiteralPath $artifactDir -Recurse -File -Filter '*-Installer.exe' |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1
        if (-not $installer) {
            throw "No NSIS installer found in $artifactDir"
        }

        if (-not $options.SkipUninstallExisting) {
            Invoke-ExistingPythonSCADUninstaller
        }

        $installDir = Join-Path $workdir 'nsis-install'
        if ($installDir -match '\s') {
            throw "NSIS silent install requires a space-free install path; rerun with --workdir under a path without whitespace: $installDir"
        }
        New-Item -ItemType Directory -Force -Path $installDir | Out-Null
        Write-SmokeLog "Installing NSIS package $($installer.Name)"
        $installProcess = Start-Process -FilePath $installer.FullName `
            -ArgumentList @('/S', "/D=$installDir") `
            -Wait `
            -PassThru
        if ($installProcess.ExitCode -ne 0) {
            throw "NSIS installer failed with exit code $($installProcess.ExitCode)"
        }
        $nsisExecutable = Find-PythonSCADExecutable -Root $installDir
        Invoke-SmokeTest -ExecutablePath $nsisExecutable -Label "NSIS $($installer.Name)" -Workdir $workdir
    }

    Write-SmokeLog 'Windows smoke tests passed'
} catch {
    $scriptFailed = $true
    throw
} finally {
    if ($options.KeepWorkdir -or $scriptFailed) {
        Write-SmokeLog "Keeping workdir: $workdir"
    } else {
        Remove-Item -LiteralPath $workdir -Recurse -Force -ErrorAction SilentlyContinue
    }
}
