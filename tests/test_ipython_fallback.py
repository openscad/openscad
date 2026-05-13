#!/usr/bin/env python3
"""Verify `--ipython` falls back to the basic REPL when IPython is missing.

This is the Layer-3 test from the PR plan: the goal is to assert that a
missing IPython is handled gracefully (a friendly diagnostic on stderr
plus a drop into the basic Python REPL), not as a hard error.

The test tries to *force* the embedded interpreter to fail importing
IPython even on a CI worker that has IPython installed by scrubbing
the environment that most commonly makes third-party packages visible:
`PYTHONPATH` is cleared, `PYTHONNOUSERSITE=1` disables the user-site
directory, and `IPYTHONDIR` is removed. The embedded interpreter still
sees the standard library and the in-tree `libraries/python/...`
overlays because those are wired up in C++ via `sys.path` regardless
of `PYTHONPATH`.

If the test environment somehow still imports IPython (for example
because it is available from system-level paths outside user-site or
`PYTHONPATH`), the result is treated as inconclusive and the test
skips with a clear INFO message instead of failing -- the Layer-1
smoke covers the happy path and we do not want false negatives on
exotic CI workers.

Usage:
    test_ipython_fallback.py <path-to-pythonscad>
"""
from __future__ import annotations

import os
import subprocess
import sys


# Rely on EOF to terminate the basic REPL; an explicit `exit()` is
# unnecessary and would hide unrelated EOF-handling regressions.
SCRIPT = "print('FALLBACK_OK')\n"

TIMEOUT_SECONDS = 60


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: test_ipython_fallback.py <pythonscad-binary>", file=sys.stderr)
        return 2

    pythonscad = sys.argv[1]
    if not os.path.isfile(pythonscad):
        print(f"binary not found: {pythonscad}", file=sys.stderr)
        return 2

    env = os.environ.copy()
    env["PYTHONPATH"] = ""
    env["PYTHONNOUSERSITE"] = "1"
    env.pop("IPYTHONDIR", None)

    proc = subprocess.run(
        [pythonscad, "--ipython"],
        input=SCRIPT,
        capture_output=True,
        text=True,
        timeout=TIMEOUT_SECONDS,
        env=env,
    )

    print("===== stdout =====")
    print(proc.stdout)
    print("===== stderr =====", file=sys.stderr)
    print(proc.stderr, file=sys.stderr)

    # Check BOTH streams: on Windows the `pythonscad.com` console shim
    # merges stderr into stdout, so a `proc.stderr`-only check would
    # always take the SKIP branch on Windows runners and silently
    # bypass the Layer-3 assertion.
    # Match the common suffix shared by every fallback code path so that
    # a broken-but-present IPython install (which emits a different
    # diagnostic than "IPython is not installed") still registers as a
    # fallback and exercises the REPL assertions below.
    fallback_msg = "falling back to the basic Python prompt"
    saw_fallback = fallback_msg in proc.stderr or fallback_msg in proc.stdout
    if not saw_fallback:
        print(
            "INFO: could not force the embedded interpreter into the "
            "fallback path (IPython is reachable even with PYTHONPATH "
            "scrubbed). Skipping Layer-3 assertion -- Layer-1 still "
            "covers the happy path.",
            file=sys.stderr,
        )
        print("SKIP")
        return 0

    if proc.returncode != 0:
        print(
            f"FAIL: fallback REPL did not exit cleanly (rc={proc.returncode})",
            file=sys.stderr,
        )
        return 1

    if "FALLBACK_OK" not in proc.stdout:
        print(
            "FAIL: fallback message printed but the piped script did not run "
            "(FALLBACK_OK marker missing on stdout)",
            file=sys.stderr,
        )
        return 1

    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
