#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  Copyright (C) 2025-2026 The OpenSCAD Developers
#  Copyright (C) 2025-2026 The PythonSCAD Developers
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
"""Install build dependencies across supported Unix flavors.

Features:
 - Detect distro & version (Debian, Ubuntu, Arch, Gentoo, Fedora, NetBSD, FreeBSD, macOS,
    openSUSE, Mageia, Solus, ALT Linux, Qomo, NixOS)
 - Configurable package lists via JSON with inheritance & version overrides
 - Dry-run & confirmation support
 - Minimal duplication ("extends" + per-version add/remove)
 - Support for pre_commands and post_commands in the config for additional setup or cleanup

Usage examples:
  scripts/uni-get-dependencies.py                                     # auto-detect and show plan (base profile)
  scripts/uni-get-dependencies.py --profile openscad-qt5              # single profile
  scripts/uni-get-dependencies.py --profile base --profile qt5        # multiple profiles combined in order
  scripts/uni-get-dependencies.py --yes                               # auto-install without prompt
  scripts/uni-get-dependencies.py --dry-run                           # show commands only
  scripts/uni-get-dependencies.py --distro fedora --version 42 --yes

Config schema (profiles/*.json):
  distros: {
    "ubuntu": { "extends": "debian", "manager": "apt", "packages": [...],
              "pre_commands": ["command1", "command2"],
              "post_commands": ["command3", "command4"],
              "version_overrides": { "24": {"add": [...], "remove": [...] } } }
  }

Version resolution order: exact VERSION_ID -> major.minor -> major.
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from typing import Dict, List, Optional, Set

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


class DistroInfo:
    def __init__(self, id: str, version: str):
        self.id = id  # normalized id (see SUPPORTED)
        self.version = version

    def __repr__(self) -> str:  # pragma: no cover - trivial
        return f"DistroInfo(id={self.id!r}, version={self.version!r})"


SUPPORTED = {"debian", "ubuntu", "arch", "gentoo", "fedora", "netbsd", "freebsd", "macos",
             "opensuse", "mageia", "solus", "altlinux", "qomo"}


def detect_distro() -> DistroInfo:
    # macOS first
    if sys.platform == "darwin":
        ver = platform.mac_ver()[0]
        return DistroInfo("macos", ver or "")

    uname_s = platform.system().lower()
    if "freebsd" in uname_s:
        ver = platform.release().split("-")[0]
        return DistroInfo("freebsd", ver)
    if "netbsd" in uname_s:
        ver = platform.release().split("-")[0]
        return DistroInfo("netbsd", ver)

    os_release = {}
    for path in ("/etc/os-release", "/usr/lib/os-release"):
        if os.path.exists(path):
            with open(path) as fh:
                for line in fh:
                    line = line.strip()
                    if not line or line.startswith('#') or '=' not in line:
                        continue
                    k, v = line.split("=", 1)
                    os_release[k] = v.strip().strip('"')
            break

    rid = os_release.get("ID", "").lower()
    version = os_release.get("VERSION_ID", "").strip()

    # Fallbacks
    if not rid and shutil.which("lsb_release"):
        try:
            rid = subprocess.check_output(["lsb_release", "-si"], text=True).strip().lower()
        except Exception:  # pragma: no cover - best effort
            pass
    if not version and shutil.which("lsb_release"):
        try:
            version = subprocess.check_output(["lsb_release", "-sr"], text=True).strip()
        except Exception:  # pragma: no cover
            pass

    # Normalize
    mapping = {
        "debian": "debian",
        "ubuntu": "ubuntu",
        "arch": "arch",
        "archlinux": "arch",
        "fedora": "fedora",
        "gentoo": "gentoo",
        "rhel": "fedora",
        "redhatenterpriseserver": "fedora",
        "opensuse": "opensuse",
        "opensuse-leap": "opensuse",
        "opensuse-tumbleweed": "opensuse",
        "sles": "opensuse",
        "suse": "opensuse",
        "mageia": "mageia",
        "solus": "solus",
        "altlinux": "altlinux",
        "qomo": "qomo"
    }
    norm = mapping.get(rid, rid)

    # Gentoo version from /etc/gentoo-release if missing
    if norm == "gentoo" and not version:
        gr = "/etc/gentoo-release"
        if os.path.exists(gr):
            txt = open(gr).read().strip()
            m = re.search(r"release (\d+\.\d+|\d+)", txt.lower())
            if m:
                version = m.group(1)
            else:
                version = "rolling"

    if norm not in SUPPORTED:
        return DistroInfo(norm or "unknown", version)
    return DistroInfo(norm, version)


def load_config(path: str) -> dict:
    with open(path) as fh:
        return json.load(fh)


def resolve_packages(cfg: dict, distro: DistroInfo) -> List[str]:
    distros = cfg["distros"]
    if distro.id not in distros:
        raise SystemExit(f"Unsupported / unknown distro '{distro.id}'. Supported: {', '.join(sorted(distros))}")

    # Inheritance chain
    chain = []
    cur = distro.id
    seen = set()
    while cur:
        if cur in seen:
            raise SystemExit(f"Inheritance loop at {cur}")
        seen.add(cur)
        d = distros[cur]
        chain.append(cur)
        cur = d.get("extends")

    # Build package set preserving order
    ordered: List[str] = []
    seen_pkg: Set[str] = set()
    for name in reversed(chain):  # base first
        pkgs = distros[name].get("packages", [])
        for p in pkgs:
            if p not in seen_pkg:
                ordered.append(p)
                seen_pkg.add(p)

    # Version overrides (apply on final distro only)
    overrides = distros[distro.id].get("version_overrides", {})
    if overrides and distro.version:
        keys_to_try = []
        v = distro.version
        keys_to_try.append(v)
        if v.count('.') >= 1:
            major_minor = '.'.join(v.split('.')[:2])
            if major_minor not in keys_to_try:
                keys_to_try.append(major_minor)
        major = v.split('.')[0]
        if major not in keys_to_try:
            keys_to_try.append(major)
        for key in keys_to_try:
            if key in overrides:
                ov = overrides[key]
                for r in ov.get("remove", []):
                    if r in ordered:
                        ordered = [x for x in ordered if x != r]
                        seen_pkg.discard(r)
                for a in ov.get("add", []):
                    if a not in seen_pkg:
                        ordered.append(a)
                        seen_pkg.add(a)
                break  # stop at first match
    return ordered


def build_commands(cfg: dict, distro: DistroInfo, packages: List[str], assume_yes: bool) -> List[List[str]]:
    d = cfg["distros"][distro.id]
    mgr = d.get("manager")
    # Inherit package manager from ancestors if not defined locally (avoids duplication)
    if not mgr:
        parent_id = d.get("extends")
        visited = set()
        while parent_id and parent_id not in visited:
            visited.add(parent_id)
            parent = cfg["distros"].get(parent_id, {})
            mgr = parent.get("manager")
            if mgr:
                break
            parent_id = parent.get("extends")
    if not mgr:
        raise SystemExit(f"No package manager defined for {distro.id} (and none found in its inheritance chain)")

    pre_cmds = [cmd.split() for cmd in d.get("pre_commands", [])]
    cmds: List[List[str]] = []
    cmds.extend(pre_cmds)
    yes_flag = []
    if mgr == "apt":
        if assume_yes:
            yes_flag = ["-y"]
        cmds.append(["sudo", "apt-get", "update"])
        cmds.append(["sudo", "apt-get", "install", *yes_flag, *packages])
    elif mgr == "dnf":
        if assume_yes:
            yes_flag = ["-y"]
        cmds.append(["sudo", "dnf", *yes_flag, "install", *packages])
    elif mgr == "pacman":
        # sync first
        cmds.append(["sudo", "pacman", "-Sy", "--needed", "--noconfirm" if assume_yes else "--needed", *packages])
    elif mgr == "emerge":
        # assume pre_commands contains sync if needed
        base = ["sudo", "emerge", "--ask=n" if assume_yes else "--ask=y", "--quiet-build" , *packages]
        cmds.append(base)
    elif mgr == "pkg":
        cmds.append(["sudo", "pkg", "install", "-y" if assume_yes else "-y", *packages])
    elif mgr == "pkgin":
        cmds.append(["sudo", "pkgin", "-y" if assume_yes else "-y", "install", *packages])
    elif mgr == "brew":
        if shutil.which("brew") is None:
            raise SystemExit("Homebrew not found. Install from https://brew.sh first.")
        cmds.append(["brew", "update"])
        cmds.append(["brew", "install", *packages])
    elif mgr == "zypper":
        if assume_yes:
            cmds.append(["sudo", "zypper", "--non-interactive", "install", *packages])
        else:
            cmds.append(["sudo", "zypper", "install", *packages])
    elif mgr == "urpmi":
        # Mageia
        cmds.append(["sudo", "urpmi", "--auto", *packages])
    elif mgr == "eopkg":
        # Solus
        cmds.append(["sudo", "eopkg", "-y", "install", *packages])
    else:
        raise SystemExit(f"Unsupported manager '{mgr}'")
    for pc in d.get("post_commands", []):
        cmds.append(pc.split())
    return cmds


def run_commands(cmds: List[List[str]], dry_run: bool):
    for cmd in cmds:
        print("$", " ".join(cmd))
        if dry_run:
            continue
        try:
            subprocess.check_call(cmd)
        except subprocess.CalledProcessError as e:
            print(f"Command failed ({e.returncode}): {' '.join(cmd)}", file=sys.stderr)
            raise


def load_profile_with_inheritance(profile_name: str, loaded_profiles: set = None) -> dict:
    """Load a profile and all its inherited profiles recursively."""
    if loaded_profiles is None:
        loaded_profiles = set()

    # Prevent circular inheritance
    if profile_name in loaded_profiles:
        raise SystemExit(f"Circular inheritance detected: {profile_name} already loaded")

    profile_path = os.path.join(SCRIPT_DIR, "deps", "profiles", f"{profile_name.lower()}.json")
    if not os.path.exists(profile_path):
        raise SystemExit(f"Profile not found: {profile_path}")

    loaded_profiles.add(profile_name)
    profile_cfg = load_config(profile_path)

    # Start with empty config
    merged_cfg = {"distros": {}}

    # Load parent profiles first (if any)
    extends = profile_cfg.get("extends", [])
    if isinstance(extends, str):
        extends = [extends]  # Support single string

    for parent_name in extends:
        parent_cfg = load_profile_with_inheritance(parent_name, loaded_profiles.copy())
        # Merge parent config into current
        for distro_name, distro_data in parent_cfg.get("distros", {}).items():
            if distro_name not in merged_cfg["distros"]:
                merged_cfg["distros"][distro_name] = {"packages": []}

            # Merge packages from parent
            merged_distro = merged_cfg["distros"][distro_name]
            parent_packages = distro_data.get("packages", [])

            # Union packages preserving order
            seen = set(merged_distro.get("packages", []))
            for pkg in parent_packages:
                if pkg not in seen:
                    merged_distro.setdefault("packages", []).append(pkg)
                    seen.add(pkg)

            # Copy other properties from parent (manager, pre_commands, etc.)
            for key, value in distro_data.items():
                if key != "packages":
                    merged_distro.setdefault(key, value)

    # Now merge current profile's packages
    for distro_name, distro_data in profile_cfg.get("distros", {}).items():
        if distro_name not in merged_cfg["distros"]:
            merged_cfg["distros"][distro_name] = {}

        merged_distro = merged_cfg["distros"][distro_name]
        current_packages = distro_data.get("packages", [])

        # Union packages preserving order
        seen = set(merged_distro.get("packages", []))
        for pkg in current_packages:
            if pkg not in seen:
                merged_distro.setdefault("packages", []).append(pkg)
                seen.add(pkg)

        # Copy/override other properties from current profile
        for key, value in distro_data.items():
            if key != "packages":
                merged_distro[key] = value

    return merged_cfg


def load_multiple_profiles(profile_names: list) -> dict:
    """Load multiple profiles and merge them in the order specified."""
    if not profile_names:
        raise SystemExit("No profiles specified")

    # Start with empty config
    final_cfg = {"distros": {}}
    active_profiles = []

    for profile_name in profile_names:
        profile_cfg = load_profile_with_inheritance(profile_name)
        active_profiles.append(profile_name.lower())

        # Merge this profile into the final config
        for distro_name, distro_data in profile_cfg.get("distros", {}).items():
            if distro_name not in final_cfg["distros"]:
                final_cfg["distros"][distro_name] = {"packages": []}

            # Merge packages from this profile
            final_distro = final_cfg["distros"][distro_name]
            profile_packages = distro_data.get("packages", [])

            # Union packages preserving order, avoiding duplicates
            seen = set(final_distro.get("packages", []))
            for pkg in profile_packages:
                if pkg not in seen:
                    final_distro.setdefault("packages", []).append(pkg)
                    seen.add(pkg)

            # Copy/override other properties from this profile
            for key, value in distro_data.items():
                if key != "packages":
                    final_distro[key] = value

    final_cfg["active_profiles"] = active_profiles
    return final_cfg


def main():
    parser = argparse.ArgumentParser(description="Install dependencies")
    parser.add_argument(
        "--profile",
        action="append",
        help="Profile name (e.g. base, openscad-qt6). Can be specified multiple times to combine profiles in order",
    )
    parser.add_argument("--distro", help="Override detected distro id")
    parser.add_argument("--version", help="Override detected distro version")
    parser.add_argument("--yes", action="store_true", help="Assume yes / non-interactive")
    parser.add_argument("--dry-run", action="store_true", help="Show commands without executing")
    parser.add_argument("--list", action="store_true", help="Only list resolved packages")
    args = parser.parse_args()

    # Default to base profile if no profiles specified
    if not args.profile:
        args.profile = ["base"]

    distro = detect_distro()
    if args.distro:
        distro.id = args.distro.lower()
    if args.version:
        distro.version = args.version

    # Load multiple profiles and combine them
    try:
        cfg = load_multiple_profiles(args.profile)
    except SystemExit as e:
        print(e, file=sys.stderr)
        return 1
    try:
        packages = resolve_packages(cfg, distro)
    except SystemExit as e:
        print(e, file=sys.stderr)
        return 1

    print(f"Detected distro: {distro.id} version: {distro.version}")
    print(f"Active profiles: {', '.join(cfg.get('active_profiles', []))}")
    print(f"Package count: {len(packages)}")
    # Always show package list for transparency; --list prints raw for scripting
    if args.list:
        for p in packages:
            print(p)
        return 0
    else:
        print("Packages to install:")
        for p in packages:
            print("  -", p)

    if not args.yes and not args.dry_run:
        reply = input("Proceed with installation? [y/N] ").strip().lower()
        if reply not in {"y", "yes"}:
            print("Aborted.")
            return 0

    cmds = build_commands(cfg, distro, packages, assume_yes=args.yes)
    run_commands(cmds, dry_run=args.dry_run)
    print("Done.")
    return 0


if __name__ == "__main__":  # pragma: no cover
    sys.exit(main())
