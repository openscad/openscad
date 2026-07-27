#!/usr/bin/env python3
"""Report or delete obsolete PythonSCAD WASM toolchain images in GHCR."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import subprocess
from pathlib import Path
from typing import Any


def gh_json(*args: str) -> Any:
    result = subprocess.run(
        ["gh", "api", *args],
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    )
    return json.loads(result.stdout)


def parse_time(value: str) -> dt.datetime:
    return dt.datetime.fromisoformat(value.replace("Z", "+00:00"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--namespace", default="pythonscad")
    parser.add_argument("--package", default="wasm-python-base")
    parser.add_argument("--protected-tags", type=Path)
    parser.add_argument("--keep-newest", type=int, default=10)
    parser.add_argument("--max-age-days", type=int, default=365)
    parser.add_argument("--untagged-max-age-days", type=int, default=14)
    parser.add_argument("--delete", action="store_true")
    args = parser.parse_args()

    protected = set()
    if args.protected_tags and args.protected_tags.exists():
        protected = {
            line.strip()
            for line in args.protected_tags.read_text(encoding="utf-8").splitlines()
            if line.strip()
        }

    endpoint = (
        f"orgs/{args.namespace}/packages/container/{args.package}/versions?per_page=100"
    )
    pages = gh_json("--paginate", "--slurp", endpoint)
    versions = [version for page in pages for version in page]
    versions.sort(
        key=lambda version: parse_time(version["updated_at"]), reverse=True
    )

    toolchain_versions = [
        version
        for version in versions
        if any(
            tag.startswith("toolchain-v1-")
            for tag in version.get("metadata", {}).get("container", {}).get("tags", [])
        )
    ]
    newest_ids = {version["id"] for version in toolchain_versions[: args.keep_newest]}

    now = dt.datetime.now(dt.timezone.utc)
    deleted = 0
    retained = 0
    for version in versions:
        tags = set(version.get("metadata", {}).get("container", {}).get("tags", []))
        days_since_update = (now - parse_time(version["updated_at"])).days

        reason: str | None
        if tags & protected:
            reason = "referenced by a protected branch or release"
        elif any(tag.startswith("buildcache-") for tag in tags):
            reason = "active registry build cache"
        elif not tags and days_since_update <= args.untagged_max_age_days:
            reason = "recent untagged manifest"
        elif not tags:
            reason = None
        elif version["id"] in newest_ids:
            reason = f"one of newest {args.keep_newest} toolchains"
        elif days_since_update <= args.max_age_days:
            reason = f"updated within the last {args.max_age_days} days"
        elif not any(tag.startswith("toolchain-v1-") for tag in tags):
            reason = "unknown non-toolchain tag"
        else:
            reason = None

        label = ",".join(sorted(tags)) if tags else "<untagged>"
        if reason:
            retained += 1
            print(f"KEEP   {version['id']} {label}: {reason}")
            continue

        action = "DELETE" if args.delete else "WOULD DELETE"
        print(
            f"{action:12} {version['id']} {label}: "
            f"{days_since_update} days since update"
        )
        if args.delete:
            subprocess.run(
                [
                    "gh",
                    "api",
                    "--method",
                    "DELETE",
                    (
                        f"orgs/{args.namespace}/packages/container/"
                        f"{args.package}/versions/{version['id']}"
                    ),
                ],
                check=True,
            )
        deleted += 1

    mode = "deleted" if args.delete else "eligible for deletion"
    print(f"\nSummary: {retained} retained, {deleted} {mode}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
