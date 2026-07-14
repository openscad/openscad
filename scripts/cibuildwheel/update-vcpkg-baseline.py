#!/usr/bin/env python3
"""Sync the vcpkg manifest baseline with the pinned vcpkg release tag."""

import json
import pathlib
import re
import subprocess


ROOT = pathlib.Path(__file__).resolve().parents[2]
INSTALL_DEPS = ROOT / "scripts" / "cibuildwheel" / "install-deps-windows.ps1"
MANIFEST = ROOT / "scripts" / "cibuildwheel" / "vcpkg.json"
VCPKG_REPO = "https://github.com/microsoft/vcpkg.git"


def pinned_vcpkg_version():
    match = re.search(r'^\$VcpkgVersion = "([^"]+)"$', INSTALL_DEPS.read_text(), re.MULTILINE)
    if not match:
        raise RuntimeError(f"Could not find $VcpkgVersion in {INSTALL_DEPS}")
    return match.group(1)


def tag_commit(version):
    ref = f"refs/tags/{version}"
    output = subprocess.check_output(
        ["git", "ls-remote", "--tags", VCPKG_REPO, ref],
        text=True,
    ).strip()
    if not output:
        raise RuntimeError(f"Could not resolve vcpkg tag {version}")
    return output.split()[0]


def main():
    version = pinned_vcpkg_version()
    commit = tag_commit(version)
    manifest = json.loads(MANIFEST.read_text())
    manifest["builtin-baseline"] = commit
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Updated {MANIFEST} builtin-baseline to {commit} for vcpkg {version}")


if __name__ == "__main__":
    main()
