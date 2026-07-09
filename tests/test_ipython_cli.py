#!/usr/bin/env python3
"""CI smoke test for `pythonscad --ipython`.

Pipes a tiny script into the binary and asserts that:
  * the process exits with code 0 within a reasonable timeout,
  * the script ran (the OK marker appears on stdout),
  * the output carries an IPython fingerprint (so we know the real
    IPython shell was used and not the fallback REPL).

When IPython is *not* installed in the test environment, the binary is
expected to print the fallback diagnostic to stderr and still execute
the piped script via the basic REPL. In that case the IPython
fingerprint check is skipped, but the OK marker check still runs.

Usage:
    test_ipython_cli.py <path-to-pythonscad>
"""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile


# NB: no trailing `exit` line. Both the basic REPL and IPython exit
# cleanly when stdin reaches EOF, and an explicit `exit` (without parens)
# would either no-op or trip IPython's "Use exit()..." advisory message,
# both of which make the test less deterministic.
#
# The leading `import ctypes, sysconfig` line exercises CPython's
# Windows sysconfig path, which used to fail in packaged MSYS2 builds
# when `sys._is_mingw` was missing.
#
# The trailing `import IPython; print('IPYTHON_VERSION ...')` line is
# the deterministic IPython fingerprint we assert on below. Earlier
# revisions of this test relied on IPython's auto-banner / `In [N]:`
# prompt to leak onto stdout, but IPython 8.x suppresses both of
# those when stdin is a pipe (only IPython 9.x emitted the version
# banner unconditionally), and any user `ipython_config.py` setting
# `display_banner = False` would tip the test over too. Probing
# `IPython.__version__` from inside the user namespace runs in BOTH
# IPython 8 and 9 with piped stdin, is unaffected by config, and
# fails loudly (ImportError) if we somehow ended up in the basic
# REPL fallback - in which case the existing fallback diagnostic on
# stderr still drives us down the SKIP-fingerprint branch instead.
SCRIPT = (
    "import ctypes, sysconfig\n"
    "from pythonscad import cube\n"
    "c = cube([1, 1, 1])\n"
    "print('OK', type(c).__name__)\n"
    # One-liner so it works in both real IPython (multi-line block
    # detection) and the basic Python REPL (multi-line blocks need a
    # blank line to close, which would force a fragile "blank line
    # consumed by IPython's auto-magic" workaround). On hosts without
    # IPython this falls into the `except` branch and prints nothing,
    # leaving stdout free of a ModuleNotFoundError traceback that
    # would otherwise pollute CI logs in the fallback path.
    "try: import IPython; print('IPYTHON_VERSION', IPython.__version__)\n"
    "except ModuleNotFoundError: pass\n"
)

TIMEOUT_SECONDS = 60


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: test_ipython_cli.py <pythonscad-binary>", file=sys.stderr)
        return 2

    pythonscad = sys.argv[1]
    if not os.path.isfile(pythonscad):
        print(f"binary not found: {pythonscad}", file=sys.stderr)
        return 2

    # Hermeticity: pin IPython config / history at a fresh empty
    # directory so a developer machine's `~/.ipython/profile_default/
    # ipython_config.py` (e.g. `display_banner = False`, custom
    # startup files that advance the prompt counter, alternative
    # input transformers, ...) cannot influence the prompt
    # fingerprints we assert on below. (Copilot review, PR #600
    # thread on tests/test_ipython_cli.py:54.) HOME is overridden to
    # the same directory so any auxiliary user config IPython or
    # its deps happen to read (`$HOME/.matplotlib`, ...) also stays
    # inside the sandbox.
    with tempfile.TemporaryDirectory(prefix="pythonscad-ipython-cli-") as sandbox:
        env = os.environ.copy()
        env["IPYTHONDIR"] = sandbox
        env["HOME"] = sandbox

        proc = subprocess.run(
            [pythonscad, "--ipython"],
            input=SCRIPT,
            env=env,
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
            f"FAIL: pythonscad --ipython exited with {proc.returncode}",
            file=sys.stderr,
        )
        return 1

    if "OK" not in proc.stdout:
        print(
            "FAIL: piped script did not run (missing 'OK' marker on stdout)",
            file=sys.stderr,
        )
        return 1

    hook_msg = "Failed calling sys.__interactivehook__"
    if hook_msg in proc.stderr or hook_msg in proc.stdout:
        print(
            "FAIL: --ipython emitted the sys.__interactivehook__ startup warning.",
            file=sys.stderr,
        )
        return 1

    # Look for the fallback diagnostic in BOTH streams: on Windows the
    # `pythonscad.com` console shim merges stderr into stdout, so a
    # `proc.stderr`-only check misses the diagnostic and the test
    # would falsely conclude that real IPython is in scope.
    # Match the common suffix shared by every fallback code path
    # ("IPython is not installed", "IPython could not be imported",
    # "IPython startup failed", argv-decode failure, ...) rather than
    # locking onto a single diagnostic message so broken-but-present
    # IPython installs are not misclassified as "real IPython running".
    fallback_msg = "falling back to the basic Python prompt"
    is_fallback = fallback_msg in proc.stderr or fallback_msg in proc.stdout

    if is_fallback:
        print(
            "INFO: IPython not available or startup failed; the fallback "
            "REPL handled the script. This is acceptable for the smoke "
            "test, but Layer-1 coverage of the real IPython prompt requires "
            "a working IPython installation.",
            file=sys.stderr,
        )
    else:
        if "IPYTHON_VERSION " not in proc.stdout:
            print(
                "FAIL: real IPython appears to be installed but the "
                "deterministic `IPYTHON_VERSION` fingerprint was not "
                "found on stdout. Either IPython failed to launch "
                "silently, the bootstrap snippet regressed, or the "
                "embedded interpreter is shadowing `import IPython` "
                "with something else.",
                file=sys.stderr,
            )
            return 1

    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
