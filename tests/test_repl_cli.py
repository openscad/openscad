#!/usr/bin/env python3
"""CI smoke test for `pythonscad --repl`.

Pipes a tiny script into the binary and asserts that:
  * the basic embedded Python REPL is reachable (exit 0 within timeout),
  * the script ran (the OK marker appears on stdout),
  * NO `IPython is not installed` diagnostic is emitted -- `--repl` is
    the explicit, intentional path to the basic REPL and must never
    print the IPython fallback warning.

Usage:
    test_repl_cli.py <path-to-pythonscad>
"""
from __future__ import annotations

import os
import subprocess
import sys


# --repl drops the user into an empty `__main__` namespace; the user
# is responsible for importing whatever they need. The explicit
# `from pythonscad import cube` line below is the contract under test
# (i.e. the basic REPL must successfully import the embedded
# `pythonscad` module). If the import regresses, the script raises
# ImportError / ModuleNotFoundError and the smoke test fails.
SCRIPT = (
    "from pythonscad import cube\n"
    "c = cube([1, 1, 1])\n"
    "print('OK', type(c).__name__)\n"
)

TIMEOUT_SECONDS = 60


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: test_repl_cli.py <pythonscad-binary>", file=sys.stderr)
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

    if "OK" not in proc.stdout:
        print(
            "FAIL: piped script did not run (missing 'OK' marker on stdout)",
            file=sys.stderr,
        )
        return 1

    # Check BOTH streams: on Windows the `pythonscad.com` console shim
    # merges stderr into stdout, so a `proc.stderr`-only check would
    # silently miss a regression where --repl started advertising the
    # IPython fallback path on Windows.
    fallback_msg = "IPython is not installed"
    if fallback_msg in proc.stderr or fallback_msg in proc.stdout:
        print(
            "FAIL: --repl emitted the IPython fallback diagnostic; the "
            "explicit --repl path must never advertise IPython.",
            file=sys.stderr,
        )
        return 1

    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
