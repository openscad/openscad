#!/usr/bin/env python3
"""Install OpenSCAD build dependencies across supported Unix flavors.

Features:
 - Detect distro & version (Debian, Ubuntu, Arch, Gentoo, Fedora, NetBSD, FreeBSD, macOS,
     openSUSE, Mageia, Solus, ALT Linux, Qomo, NixOS)
 - Configurable package lists via JSON with inheritance & version overrides
 - Dry-run & confirmation support
 - Minimal duplication ("extends" + per-version add/remove)

Usage examples:
  scripts/install-deps.py              # auto-detect and show plan
  scripts/install-deps.py --yes        # auto-install without prompt
  scripts/install-deps.py --dry-run    # show commands only
  scripts/install-deps.py --distro fedora --version 42 --yes

Config schema (packages.json):
  distros: {
    "ubuntu": { "extends": "debian", "manager": "apt", "packages": [...],
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
CONFIG_PATH = os.path.join(SCRIPT_DIR, "deps", "packages.json")


class DistroInfo:
    def __init__(self, id: str, version: str):
        self.id = id  # normalized id (see SUPPORTED)
        self.version = version

    def __repr__(self) -> str:  # pragma: no cover - trivial
        return f"DistroInfo(id={self.id!r}, version={self.version!r})"


SUPPORTED = {"debian", "ubuntu", "arch", "gentoo", "fedora", "netbsd", "freebsd", "macos",
             "opensuse", "mageia", "solus", "altlinux", "qomo", "nixos"}


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
        "qomo": "qomo",
        "nixos": "nixos"
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
    elif mgr == "nix-env":
        # NixOS: prefer user-level install; advise flakes separately
        attr_pkgs = []
        for p in packages:
            if p.startswith("nixpkgs."):
                attr_pkgs.append(p)
            else:
                attr_pkgs.append(f"nixpkgs.{p}")
        cmds.append(["nix-env", "-iA", *attr_pkgs])
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


def main():
    parser = argparse.ArgumentParser(description="Install OpenSCAD (or fork) dependencies")
    parser.add_argument("--config", default=CONFIG_PATH, help="Base config (shared) JSON path")
    parser.add_argument("--profile", default="openscad", help="Profile name (e.g. openscad, pythonscad)")
    parser.add_argument("--distro", help="Override detected distro id")
    parser.add_argument("--version", help="Override detected distro version")
    parser.add_argument("--yes", action="store_true", help="Assume yes / non-interactive")
    parser.add_argument("--dry-run", action="store_true", help="Show commands without executing")
    parser.add_argument("--list", action="store_true", help="Only list resolved packages")
    args = parser.parse_args()

    distro = detect_distro()
    if args.distro:
        distro.id = args.distro.lower()
    if args.version:
        distro.version = args.version

    cfg = load_config(args.config)
    # Merge in profile file if present (scripts/deps/profiles/<profile>.json)
    profile_path = os.path.join(SCRIPT_DIR, "deps", "profiles", f"{args.profile.lower()}.json")
    if os.path.exists(profile_path):
        profile_cfg = load_config(profile_path)
        # Profile distros override/extend base distros
        base_distros = cfg.setdefault("distros", {})
        for name, data in profile_cfg.get("distros", {}).items():
            if name in base_distros:
                # Merge: keep existing unless overridden; merge package lists if specified
                merged = dict(base_distros[name])
                for k, v in data.items():
                    if k == "packages" and k in merged:
                        # union preserving order
                        seen = set(merged[k])
                        for pkg in v:
                            if pkg not in seen:
                                merged[k].append(pkg)
                                seen.add(pkg)
                    else:
                        merged[k] = v
                base_distros[name] = merged
            else:
                base_distros[name] = data
        cfg["active_profile"] = args.profile.lower()
    else:
        cfg["active_profile"] = "default"
    try:
        packages = resolve_packages(cfg, distro)
    except SystemExit as e:
        print(e, file=sys.stderr)
        return 1

    print(f"Detected distro: {distro.id} version: {distro.version}")
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
