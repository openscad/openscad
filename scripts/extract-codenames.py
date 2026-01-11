#!/usr/bin/env python3
"""
Extract distribution codenames from supported-distributions.json

Usage:
    extract-codenames.py <path_to_supported_distributions_json>

This script reads the supported-distributions.json file and outputs
the codenames of all Debian and Ubuntu distributions, one per line,
sorted alphabetically.
"""

import json
import sys

def main():
    if len(sys.argv) < 2:
        print("Error: Missing path to supported-distributions.json", file=sys.stderr)
        sys.exit(1)

    json_path = sys.argv[1]

    try:
        with open(json_path) as f:
            config = json.load(f)
    except FileNotFoundError:
        print(f"Error: File not found: {json_path}", file=sys.stderr)
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in {json_path}: {e}", file=sys.stderr)
        sys.exit(1)

    codenames = set()
    for dist in config.get("distributions", []):
        if dist.get("family") in ["debian", "ubuntu"]:
            codename = dist.get("codename")
            if codename:
                codenames.add(codename)

    for codename in sorted(codenames):
        print(codename)

if __name__ == "__main__":
    main()
