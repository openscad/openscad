#!/usr/bin/env python3
"""
Validate DXF export output using ezdxf.
Usage: export_dxf_ezdxf_test.py --openscad=<path> <scad_file> [extra args...]

The script:
  1. Exports the .scad file to a .dxf using OpenSCAD with the given extra args
  2. Runs `ezdxf info` on the result and fails if ezdxf reports errors
"""

import argparse
import os
import subprocess
import sys
import tempfile


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--openscad", required=True, help="Path to OpenSCAD binary")
    # Accept unknown args so CMake test infra can pass extra flags like
    # --format=DXF --render=force -O export-dxf/version=R14
    args, remaining = parser.parse_known_args()
    return args, remaining


def main():
    args, extra_args = parse_args()

    # The last positional arg that looks like a .scad file is our input
    scad_files = [a for a in extra_args if a.endswith(".scad")]
    other_args = [a for a in extra_args if not a.endswith(".scad")]

    if not scad_files:
        print("ERROR: No .scad file found in arguments", file=sys.stderr)
        sys.exit(1)

    failed = []
    for scad_file in scad_files:
        with tempfile.NamedTemporaryFile(suffix=".dxf", delete=False) as tmp:
            out_dxf = tmp.name

        try:
            # Step 1: export DXF
            cmd = [args.openscad, "-o", out_dxf] + other_args + [scad_file]
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"FAIL (openscad): {scad_file}", file=sys.stderr)
                print(result.stderr, file=sys.stderr)
                failed.append(scad_file)
                continue

            # Step 2: validate with ezdxf info
            try:
                import ezdxf
                from ezdxf import recover
                doc, auditor = recover.readfile(out_dxf)
                if auditor.has_errors:
                    print(f"FAIL (ezdxf): {scad_file}", file=sys.stderr)
                    for err in auditor.errors:
                        print(f"  {err}", file=sys.stderr)
                    failed.append(scad_file)
                else:
                    print(f"OK: {scad_file}")
            except ImportError:
                print("ERROR: ezdxf not installed. Install with: pip install ezdxf",
                      file=sys.stderr)
                sys.exit(2)
        finally:
            if os.path.exists(out_dxf):
                os.unlink(out_dxf)

    if failed:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
