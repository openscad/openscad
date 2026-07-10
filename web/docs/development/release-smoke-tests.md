# Release Smoke Tests

The release smoke scripts check that packaged deployables are basically sound
before publishing a release. They exercise the end-user packages on their target
platforms and look for obvious packaging failures: missing shared libraries,
broken Python startup, unusable REPL entry points, or failed exports.

They are not intended to replace ctests. Detailed geometry and API behavior,
such as whether a modeling function computes the exact expected result, is
covered by the build and regression test suite.

## What Is Tested

Each deployable is tested in an isolated temporary directory:

- `pythonscad --info` starts without crashing.
- A plain OpenSCAD file containing `cube(10);` exports to STL.
- A Python script using `from pythonscad import *` exports to STL with
  `--trust-python`.
- `pythonscad --repl` accepts a small piped Python script and exports a cube.
- `pythonscad --ipython` runs a small Python script and exports a cube.

The GUI smoke test is still manual for now. MSIX and arm64 deployables are also
out of scope for the first automated pass.

## Required Tools

Install the tools needed for the platform you are testing:

- `gh`, authenticated for the `pythonscad/pythonscad` repository.
- Docker, `python3`, and `realpath`, for testing `.deb` and `.rpm` packages
  on Linux.
- Bash, for Linux and macOS scripts.
- PowerShell, for the Windows script.

The scripts download artifacts from GitHub Actions by default. They can also run
against already-downloaded deployables with `--skip-download --artifact-dir`.

## Trigger Build Workflows

From a Linux checkout, dispatch all packaging workflows:

```bash
scripts/release-smoke/trigger-release-builds.sh
```

By default, this uses the Release Please branch
`release-please--branches--master--components--pythonscad` when it exists, and
falls back to `master` otherwise. Override the ref explicitly when needed:

```bash
scripts/release-smoke/trigger-release-builds.sh --ref master
```

For a real release upload rehearsal, pass the release tag:

```bash
scripts/release-smoke/trigger-release-builds.sh --upload-to-release v0.20.0
```

Wait for the workflows to finish before running the platform smoke scripts.
To have the trigger script wait and report progress automatically, pass
`--wait`:

```bash
scripts/release-smoke/trigger-release-builds.sh --wait
```

With `--wait`, the script polls once per minute, prints each workflow as it
finishes, and exits non-zero if any triggered workflow fails.

## Linux

Test AppImages on a Linux machine:

```bash
scripts/release-smoke/test-appimages.sh
```

Test `.deb` and `.rpm` packages in Docker containers:

```bash
scripts/release-smoke/test-linux-packages.sh
```

The package script maps each downloaded package to its Docker image using
`supported-distributions.json`, installs the package, installs distro IPython
packages, then runs the shared smoke checks.

## Windows

On a Windows 11 development VM, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\release-smoke\test-windows.ps1
```

The script tests the ZIP distribution in place. It also uninstalls existing
PythonSCAD installations found in the standard uninstall registry locations,
installs the NSIS package silently into a temporary directory, and runs the same
smoke checks against the installed executable.

To skip installer testing and only test the ZIP:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\release-smoke\test-windows.ps1 --skip-nsis-install
```

MSIX is intentionally skipped for now because unsigned MSIX installation is
policy-dependent and may require Developer Mode, certificate setup, or elevated
PowerShell.

## macOS

On macOS, run:

```bash
scripts/release-smoke/test-macos.sh
```

The script downloads the latest DMG artifact, mounts it read-only, locates the
`.app` bundle, and runs the smoke checks against the executable inside
`Contents/MacOS`.

## Common Options

The scripts use the same option names where practical:

- `--repo <owner/name>` selects the GitHub repository.
- `--ref <branch-or-tag>` filters latest workflow runs or selects dispatch ref.
- `--run-id <id>` tests a specific workflow run when a script uses one workflow.
- `--wait` waits for dispatched workflow runs when using
  `trigger-release-builds.sh`.
- `--workdir <path>` uses an explicit working directory.
- `--keep-workdir` preserves temporary files and logs for debugging.
- `--skip-download` uses deployables already present locally.
- `--artifact-dir <path>` points to already-downloaded artifacts.
- `--help` prints script-specific usage.

Some scripts have extra platform-specific flags. For example,
`test-linux-packages.sh` accepts `--deb-run-id` and `--rpm-run-id`, and
`test-windows.ps1` accepts `--skip-nsis-install`.

## Troubleshooting

Use `--keep-workdir` first. Each smoke check writes command logs next to its
temporary input files, which makes it easier to see whether a failure came from
startup, export, REPL, or IPython.

If no artifacts are found, confirm that the matching workflow completed
successfully and that the script is looking at the intended branch or run id.
Use `--run-id` or the package-specific run-id flags to remove ambiguity.
