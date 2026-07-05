#!/usr/bin/env python3
"""CI smoke test for PythonSCAD's version helpers."""
from __future__ import annotations

import os
import subprocess
import sys


SCRIPT = (
    "from pythonscad import version, version_num, version_string\n"
    "v = version()\n"
    "n = version_num()\n"
    "s = version_string()\n"
    "print('VERSION_HELPERS', repr(v), repr(n), repr(s))\n"
    "assert len(v) == 3\n"
    "assert all(isinstance(part, int) for part in v)\n"
    "assert isinstance(n, int)\n"
    "assert isinstance(s, str)\n"
    "assert n == v[0] * 1000000 + v[1] * 1000 + v[2]\n"
    "assert s.startswith(f'{v[0]}.{v[1]}.{v[2]}')\n"
)

TIMEOUT_SECONDS = 60


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: test_version_helpers.py <pythonscad-binary>", file=sys.stderr)
        return 2

    pythonscad = sys.argv[1]
    if not os.path.isfile(pythonscad):
        print(f"binary not found: {pythonscad}", file=sys.stderr)
        return 2

    proc = subprocess.run(
        [pythonscad, "--repl"],
        input=SCRIPT,
        capture_output=True,
        text=True,
        timeout=TIMEOUT_SECONDS,
    )

    print("===== stdout =====")
    print(proc.stdout)
    print("===== stderr =====", file=sys.stderr)
    print(proc.stderr, file=sys.stderr)

    if proc.returncode != 0:
        print(
            f"FAIL: pythonscad --repl exited with {proc.returncode}",
            file=sys.stderr,
        )
        return 1

    if "VERSION_HELPERS" not in proc.stdout:
        print(
            "FAIL: piped script did not run (missing VERSION_HELPERS marker)",
            file=sys.stderr,
        )
        return 1

    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
