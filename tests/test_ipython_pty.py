#!/usr/bin/env python3
"""Layer-2 (PTY) smoke test for ``pythonscad --ipython``.

The Layer-1 smoke test (``test_ipython_cli.py``) talks to the binary
through plain pipes, which is enough to assert that:
  * the process exits cleanly,
  * the script ran (the OK marker appears on stdout),
  * an IPython fingerprint shows up on stdout.

Pipes are however a *non-interactive* stdin from the embedded
interpreter's point of view. IPython itself detects this and silently
disables interactive features such as the ``In [1]:`` prompt, tab
completion, and prompt_toolkit input handling. The Layer-1 test
therefore cannot catch regressions that only manifest in real
interactive use.

Layer-2 closes this gap by allocating a pseudo-terminal (PTY) and
driving IPython through it as if the user were typing. We assert:

  * the IPython banner is visible (so we know the real prompt is up),
  * the ``In [1]:`` interactive prompt appears,
  * a typed expression produces the expected ``Out[...]`` echo.

When IPython is not installed in the test environment, the binary
falls back to the basic REPL. This test then checks the fallback path
instead (``>>>`` prompt + correct expression evaluation).

This test is automatically skipped when ``pexpect`` is unavailable.
``pexpect`` is a third-party PyPI package, NOT part of the Python
standard library, and is frequently absent on minimal runners. CI
must install it ahead of time (e.g. ``pip install pexpect``) to
enable PTY coverage; otherwise the test self-skips with an INFO
diagnostic and Layer-1 / Layer-3 still cover the non-PTY paths.

Usage:
    test_ipython_pty.py <path-to-pythonscad>
"""
from __future__ import annotations

import os
import sys
import tempfile


# pexpect is the standard way to drive a child process through a PTY
# on POSIX. Importing it lazily so that the script can announce a
# clean SKIP on hosts that lack it (e.g. minimal CI images).
try:
    import pexpect  # type: ignore
except ImportError:
    pexpect = None  # type: ignore[assignment]


# Skip on Windows: pexpect's PTY support is POSIX-only. The non-PTY
# Layer-1 smoke test still covers the Windows runner.
SKIP_PLATFORM = sys.platform == "win32"


# Generous timeouts for slow CI runners. IPython startup pulls in a
# large dependency graph (jedi, prompt_toolkit, ...) so 60s is a
# realistic upper bound on a busy x86_64 worker.
STARTUP_TIMEOUT = 60
COMMAND_TIMEOUT = 30


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: test_ipython_pty.py <pythonscad-binary>", file=sys.stderr)
        return 2

    pythonscad = sys.argv[1]
    if not os.path.isfile(pythonscad):
        print(f"binary not found: {pythonscad}", file=sys.stderr)
        return 2

    if SKIP_PLATFORM:
        # CTest treats stdout-only "SKIP" as a pass with the
        # `SKIP_REGULAR_EXPRESSION` test property, so we just print the
        # marker and return 0 here. The CMakeLists entry sets the
        # property accordingly. The wording deliberately blames the
        # test design (POSIX-only) rather than `pexpect`'s availability
        # because `pexpect` may well be installed on a Windows runner
        # and the skip is still correct.
        print("SKIP: PTY-based test is POSIX-only and is not supported on win32")
        return 0

    if pexpect is None:
        print(
            "SKIP: pexpect is not installed; Layer-2 PTY coverage requires it. "
            "Install with `pip install pexpect` or `apt install python3-pexpect`."
        )
        return 0

    # Hermeticity: pin IPython config / history at a fresh empty
    # directory so a developer machine's `~/.ipython/profile_default/
    # ipython_config.py` cannot influence the `In [1]:` /
    # `In [2]:` prompt indices we match on, banner visibility, or
    # input transformers. Without this the test would fail outside
    # CI on any machine whose user IPython config advances the
    # prompt counter (startup files), disables the banner, etc.
    # (Copilot review, PR #600 thread on tests/test_ipython_pty.py:103.)
    # HOME is overridden to the same directory so auxiliary user
    # config IPython or its deps happen to read also stays in the
    # sandbox. The `with` block scope spans the whole interaction
    # so the sandbox dir is cleaned up only after the child has
    # exited and we've validated its output.
    with tempfile.TemporaryDirectory(prefix="pythonscad-ipython-pty-") as sandbox:
        env = os.environ.copy()
        env["IPYTHONDIR"] = sandbox
        env["HOME"] = sandbox

        # Spawn the binary attached to a fresh PTY. encoding='utf-8'
        # matches our `-Xutf8=1` invocation pattern elsewhere.
        child = pexpect.spawn(
            pythonscad,
            args=["--ipython"],
            encoding="utf-8",
            timeout=STARTUP_TIMEOUT,
            # Disable the readline/echo that would otherwise pollute the
            # captured transcript with our own typed bytes.
            echo=False,
            env=env,
        )
        # Tee the PTY transcript to stderr so a CI failure shows what
        # IPython actually produced on the way to the timeout/mismatch.
        child.logfile_read = sys.stderr

        return _drive_pty(child, pythonscad)


def _drive_pty(child, pythonscad: str) -> int:
    """Run the interactive scenario; returns the exit code for ctest.

    Split out so that the `tempfile.TemporaryDirectory` context manager
    cleanly wraps spawn + drive + close while keeping the linear
    "expect / sendline / expect" flow readable. Any control-flow
    `return` from this function still triggers the sandbox cleanup
    via the caller's `with` block exit.

    `pythonscad` is woven into every FAIL diagnostic so a CI matrix
    failure log identifies which binary's interactive contract
    regressed at a glance, instead of "the PTY test failed
    somewhere".
    """
    try:
        # First, decide whether we got the real IPython or the fallback.
        # Both diagnostics are deterministic strings emitted shortly
        # after process start.
        idx = child.expect(
            [
                # IPython 8/9 banner. We match the bracketed `In [N]:`
                # prompt rather than the version string to stay
                # tolerant of upstream banner wording changes.
                # The \S* in the index slot tolerates ANSI color
                # escape codes that IPython interleaves around the
                # digits when stdout is a TTY (e.g. with the
                # default `Linux` color scheme on a real PTY).
                r"In \[\S*1\S*\]:",
                # Fallback REPL prompt - basic Python uses `>>> `.
                r">>> ",
                # Friendly diagnostic emitted by diagnose_failed_ipython_import
                r"IPython is not installed",
            ],
            timeout=STARTUP_TIMEOUT,
        )
    except pexpect.TIMEOUT:
        print(
            f"FAIL: {pythonscad}: timed out waiting for either IPython "
            "banner, fallback prompt, or fallback diagnostic on the PTY.",
            file=sys.stderr,
        )
        try:
            child.close(force=True)
        except Exception:
            pass
        return 1

    used_ipython = idx == 0
    used_fallback_prompt = idx == 1
    saw_fallback_diag = idx == 2

    if saw_fallback_diag:
        # We saw the diagnostic but not yet the prompt. Wait for it.
        try:
            child.expect(r">>> ", timeout=STARTUP_TIMEOUT)
            used_fallback_prompt = True
        except pexpect.TIMEOUT:
            print(
                f"FAIL: {pythonscad}: saw 'IPython is not installed' "
                "diagnostic but the fallback REPL prompt never appeared.",
                file=sys.stderr,
            )
            child.close(force=True)
            return 1

    # Now drive the prompt with a tiny expression and verify the echo.
    if used_ipython:
        # Real IPython echoes the expression result on a line that
        # starts with `Out[N]:`. We deliberately do NOT match the
        # exact `Out[N]:` literal because IPython 8/9 wraps the index
        # in ANSI color codes (e.g. `Out[\x1b[0;1m2\x1b[0m]:`), and
        # the embedded escape sequences would defeat a literal regex.
        # The `'PyOpenSCAD'` quoted repr is plain text and uniquely
        # identifies the expression result, so matching that is
        # sufficient + robust across IPython color-scheme changes.
        child.sendline("from pythonscad import cube")
        # Wait for the next prompt; same ANSI caveat applies, so we
        # match the prompt's bracket structure rather than its index.
        child.expect(r"In \[\S*2\S*\]:", timeout=COMMAND_TIMEOUT)
        child.sendline("type(cube([1, 1, 1])).__name__")
        child.expect(r"'PyOpenSCAD'", timeout=COMMAND_TIMEOUT)
        child.sendline("exit()")
    elif used_fallback_prompt:
        # Basic REPL: no `Out[N]:` echo, but expression evaluation still
        # prints the value. The `__main__` namespace starts empty (no
        # auto-import), so we explicitly import `cube` first and then
        # exercise it. The basic REPL doesn't echo successful
        # statements, so we just wait for the next `>>> ` prompt to
        # know the import landed before sending the next line.
        child.sendline("from pythonscad import cube")
        child.expect(r">>> ", timeout=COMMAND_TIMEOUT)
        child.sendline("print(type(cube([1, 1, 1])).__name__)")
        child.expect(r"PyOpenSCAD", timeout=COMMAND_TIMEOUT)
        child.sendline("exit()")
    else:  # pragma: no cover - defensive
        print(
            f"FAIL: {pythonscad}: internal logic error: no prompt "
            "path was taken.",
            file=sys.stderr,
        )
        child.close(force=True)
        return 1

    # Wait for the child to exit, with a generous timeout. Any
    # remaining output (Goodbye banner, EOF) is fine - we already
    # validated the interactive contract.
    child.expect(pexpect.EOF, timeout=COMMAND_TIMEOUT)
    child.close()

    if child.exitstatus is None:
        print(
            f"FAIL: {pythonscad}: child exited via signal "
            f"{child.signalstatus}, expected normal exit.",
            file=sys.stderr,
        )
        return 1
    if child.exitstatus != 0:
        print(
            f"FAIL: {pythonscad}: child exit status "
            f"{child.exitstatus}, expected 0.",
            file=sys.stderr,
        )
        return 1

    if used_ipython:
        print("PASS: real IPython interactive prompt validated through PTY")
    else:
        print("PASS: basic REPL fallback prompt validated through PTY")
    return 0


if __name__ == "__main__":
    sys.exit(main())
