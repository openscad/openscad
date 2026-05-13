#!/usr/bin/env python3
"""Layer-4 precedence test for the bundled-IPython fallback.

Background
----------
PythonSCAD's binary distributions (AppImage / macOS .app / Windows
installer) bundle IPython under a ``pythonscad-bundled-py`` directory
that ``initPython`` appends to ``sys.path`` at startup.  The
``initPython`` code in ``src/python/pyopenscad.cc`` deliberately uses
``PyList_Append`` (not ``insert(0, ...)``) so that the bundled directory
sits at the END of ``sys.path``.  This way:

  * If the user has IPython installed in their interpreter / venv /
    --user site / distro package, that copy wins (because it sits
    earlier on ``sys.path``).
  * Only when nothing else provides IPython does the bundled fallback
    kick in.

This test asserts the structural invariant that makes the precedence
work: every directory on ``sys.path`` that contains
``pythonscad-bundled-py`` is positioned strictly *after* every other
directory that ships an importable ``IPython``.

The check is intentionally narrow: it does not require IPython to be
installed at all.  When neither a bundle nor a user IPython exists,
the test passes trivially (and prints ``SKIP-NORMAL`` so a CI log
shows precisely why).

Usage:
    test_ipython_precedence.py <path-to-pythonscad>
"""
from __future__ import annotations

import json
import os
import subprocess
import sys


# We push a small probe script into the binary's stdin via --repl so
# we can introspect the post-init sys.path. --repl is the cheapest
# option here: it just calls initPython() and runs stdin, so the
# resulting sys.path includes both the user environment and the
# bundled fallback path appended by initPython.
PROBE = r"""
import json, os, sys

def _has_ipython(d):
    # We treat "this directory provides an IMPORTABLE IPython" as
    # "any of these canonical Python-import layouts is present".
    # We deliberately do NOT actually import IPython here - that
    # would have side effects we'd rather avoid in a precedence test.
    #
    # The `os.path.isdir(...)` check on its own is a false positive:
    # an empty directory named `IPython/` (e.g. a stray namespace
    # package or a packaging artifact) would wrongly mark a sys.path
    # entry as an IPython provider, which can flip the precedence
    # comparison. We require a real package marker:
    #
    #   * `IPython/__init__.py` -- regular installed package, source
    #     checkout, or pip --target=DIR layout.
    #   * `IPython.py` -- single-file install (rare but legal).
    #   * `IPython-*.dist-info/` -- pip-installed wheel with
    #     `__init__.py` reachable; the dist-info is the
    #     authoritative marker even on namespace-package layouts.
    if os.path.isfile(os.path.join(d, "IPython", "__init__.py")):
        return True
    if os.path.isfile(os.path.join(d, "IPython.py")):
        return True
    try:
        entries = os.listdir(d)
    except OSError:
        return False
    for name in entries:
        # The wheel metadata directory is the authoritative pip-install
        # marker. PEP 503 distribution-name normalization is
        # lower-case, so modern pip lays it out as
        # `ipython-9.13.0.dist-info` even though the importable
        # package keeps the capital `IPython`. Some layouts (older
        # pip, hand-built wheels, distro repacks) preserve the
        # capital form `IPython-9.13.0.dist-info`. Match both by
        # comparing case-insensitively on the project-name prefix.
        lower = name.lower()
        if lower.startswith("ipython-") and lower.endswith(".dist-info"):
            return True
    return False

bundled = []
ipython_providers = []
for i, p in enumerate(sys.path):
    # `pythonscad-bundled-py` is the leaf name we use in the bundler;
    # match anywhere in the path so platform-specific prefixes
    # (Frameworks/, lib/) are accepted.
    if "pythonscad-bundled-py" in p:
        bundled.append((i, p))
    if _has_ipython(p):
        ipython_providers.append((i, p))

# Emit a deterministic JSON line so the parent test harness can parse
# it without depending on an interactive prompt.
print("PYTHONSCAD_PRECEDENCE_REPORT=" + json.dumps({
    "bundled": bundled,
    "ipython_providers": ipython_providers,
}))
"""

TIMEOUT_SECONDS = 60


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: test_ipython_precedence.py <pythonscad-binary>", file=sys.stderr)
        return 2

    pythonscad = sys.argv[1]
    if not os.path.isfile(pythonscad):
        print(f"binary not found: {pythonscad}", file=sys.stderr)
        return 2

    proc = subprocess.run(
        [pythonscad, "--repl"],
        input=PROBE,
        capture_output=True,
        text=True,
        timeout=TIMEOUT_SECONDS,
    )

    if proc.returncode != 0:
        print(
            f"FAIL: pythonscad --repl exited with {proc.returncode}",
            file=sys.stderr,
        )
        print("===== stdout =====", file=sys.stderr)
        print(proc.stdout, file=sys.stderr)
        print("===== stderr =====", file=sys.stderr)
        print(proc.stderr, file=sys.stderr)
        return 1

    # Find the JSON line. There may be other prompt noise around it.
    report = None
    for line in proc.stdout.splitlines():
        if line.startswith("PYTHONSCAD_PRECEDENCE_REPORT="):
            report = json.loads(line.split("=", 1)[1])
            break

    if report is None:
        print(
            "FAIL: could not find PYTHONSCAD_PRECEDENCE_REPORT line on stdout.",
            file=sys.stderr,
        )
        print("===== stdout =====", file=sys.stderr)
        print(proc.stdout, file=sys.stderr)
        return 1

    bundled = report["bundled"]
    providers = report["ipython_providers"]

    print(f"Bundle entries on sys.path: {len(bundled)}")
    for idx, p in bundled:
        print(f"  [sys.path[{idx}]] {p}")
    print(f"IPython providers on sys.path: {len(providers)}")
    for idx, p in providers:
        print(f"  [sys.path[{idx}]] {p}")

    if not bundled:
        # Dev-machine builds do not ship the bundled fallback. The
        # invariant is vacuously true; record an explicit SKIP so a
        # human reading the log knows why.
        print(
            "SKIP-NORMAL: this build has no pythonscad-bundled-py on "
            "sys.path; precedence invariant is vacuously satisfied. "
            "Bundle-shipping AppImage / macOS / Windows builds "
            "exercise this test for real."
        )
        return 0

    # The contract: every bundle entry must come AFTER every non-bundle
    # IPython provider. Equivalently: the minimum bundle index must be
    # greater than the maximum non-bundle provider index.
    bundle_indices = {idx for idx, _ in bundled}
    non_bundle_provider_indices = [
        idx for idx, _ in providers if idx not in bundle_indices
    ]

    if not non_bundle_provider_indices:
        # Only the bundle provides IPython on this run. The invariant
        # cannot be violated; this is the "fallback actually used"
        # case the bundle was designed for.
        print(
            "PASS: bundled IPython is the sole provider on sys.path; "
            "no other entry can shadow it."
        )
        return 0

    min_bundle_idx = min(bundle_indices)
    max_user_idx = max(non_bundle_provider_indices)

    if max_user_idx >= min_bundle_idx:
        print(
            "FAIL: bundled IPython directory at sys.path index "
            f"{min_bundle_idx} comes before a user IPython provider at "
            f"index {max_user_idx}. The bundle would shadow the user's "
            "IPython, breaking the precedence contract documented in "
            "src/python/pyopenscad.cc.",
            file=sys.stderr,
        )
        return 1

    print(
        f"PASS: bundle index {min_bundle_idx} > user-provider index "
        f"{max_user_idx}. User-installed IPython wins over the bundle."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
