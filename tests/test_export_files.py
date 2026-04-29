#!/usr/bin/env python3
#
# Regression test driver for fixtures that produce one or more output files
# via in-script PythonSCAD `export()` calls (including helpers like
# `MultiToolExporter`).
#
# The driver:
#
#   1. Creates a per-test scratch directory (`output/<testname>/<basename>/`),
#      wiping any leftover content from a previous run so the comparison
#      below can never be polluted by stale files.
#   2. Runs the PythonSCAD binary with the fixture as a script, with that
#      scratch directory as CWD, so any `export("foo.stl")` call lands
#      there. A throwaway `-o <tmp>/_cli_driver_dummy.echo` is supplied
#      to force CLI / headless mode; without it, PythonSCAD treats the
#      script as "open in GUI" and hits the single-instance lock if a
#      desktop pythonscad is already running. The dummy is written into
#      a private `tempfile.TemporaryDirectory()` so the rundir only ever
#      contains the artifacts the fixture actually produces.
#   3. Walks the scratch directory recursively and treats every regular
#      file -- across any extension and any subdirectory -- as a fixture
#      output to be checked. Coverage-instrumentation artifacts emitted
#      by the binary itself (the ``.gcov/`` subtree from
#      ``-fprofile-dir=.gcov`` plus stray ``*.gcda`` / ``*.gcno`` files)
#      are skipped because they are runtime side-effects of
#      ``-DPROFILE=ON`` builds, not fixture outputs.
#   4. Applies format-aware post-processing (header progname rewrite for
#      STL/SVG/OBJ, inner-XML extraction for 3MF; other extensions pass
#      through untouched) keyed by each file's own extension, then
#      compares each produced file against
#      `tests/regression/<testname>/<basename>/<rel-path>` using
#      `test_cmdline_tool.compare_default()` -- a normalized text
#      comparison (line-ending normalization + unified diff), not a raw
#      bytes-equality check. For the ASCII / text-derived formats that
#      the post-processors normalize, this is effectively
#      bytes-equality; true binary outputs (binary STL, AMF, ...) would
#      need a separate bytes-equality branch added to `_post_process`
#      and the comparison step. The expected directory mirrors the
#      actual directory layout one-for-one (relative path is the key on
#      both sides), so missing or unexpected files are caught by simple
#      set diffing.
#
# Because discovery is "every file under rundir", a single fixture can
# legitimately mix formats (e.g. a `MultiToolExporter` writing
# `parts/red.stl` + `parts/blue.stl` alongside an assembly
# `assembly.3mf`) and they all get checked together with no extra
# wiring.
#
# When the TEST_GENERATE environment variable is set to any non-empty
# value (or -g/--generate is passed), the produced files are copied into
# place as the new goldens instead of being compared, and any stale
# files under `expecteddir` that the fixture no longer writes are
# removed so the expected tree always mirrors the run output. Note:
# this matches the existing convention in `test_cmdline_tool.py` and
# `test_pretty_print.py`, which also accept any non-empty value (the
# recommended idiom is `TEST_GENERATE=1`, but `TEST_GENERATE=0` would
# also enable generation -- unset the variable to disable instead).
#
# Usage:
#   test_export_files.py
#       --pythonscad <pythonscad-binary>
#       --testname <test-group-name>
#       --basename <fixture-basename>
#       [--regressiondir <dir>]
#       [--generate]
#       <fixture.py> [<extra-args-for-pythonscad>...]
#
# Exit codes match test_cmdline_tool.py: 0 pass, 1 failure, 2 invalid args.

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_cmdline_tool as tct  # noqa: E402  reuse helpers and globals


def _setup_tct_options():
    """Populate the globals that ``tct.compare_default`` reads from."""
    tct.options = tct.Options()
    tct.options.exclude_line_re = None
    tct.options.exclude_debug = False


# Format-aware post-processors, keyed by lowercase file extension
# (including the leading dot). Files whose extension is not listed pass
# through untouched and are compared as-is by ``tct.compare_default``,
# which already does line-ending-tolerant text comparison. Add a new
# entry here if a future format requires extra normalization.
_POST_PROCESSORS = {
    ".stl": tct.post_process_progname,
    ".svg": tct.post_process_progname,
    ".obj": tct.post_process_progname,
    ".3mf": tct.post_process_3mf,
}


def _post_process(path):
    """Normalize ``path`` in place based on its extension, if known."""
    fn = _POST_PROCESSORS.get(Path(path).suffix.lower())
    if fn is not None:
        fn(str(path))


def _run_pythonscad(pythonscad, fixture, extra_args, rundir):
    """Run the binary inside ``rundir`` so in-script ``export()`` writes there.

    A dummy ``-o`` is supplied to force CLI/headless mode; without it,
    PythonSCAD treats the script as "open this file in the GUI" and hits
    the single-instance lock if a desktop pythonscad is already running.
    The dummy output is routed into a private ``TemporaryDirectory`` so
    it never lands in ``rundir`` (which then only ever contains the
    artifacts the fixture actually produces) and is cleaned up
    automatically when the context manager exits.
    """
    fontdir = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "data/ttf"))
    env = os.environ.copy()
    env["OPENSCAD_FONT_PATH"] = fontdir

    with tempfile.TemporaryDirectory(prefix="pythonscad-cli-dummy-") as tmp:
        cli_dummy_path = os.path.join(tmp, "_cli_driver_dummy.echo")
        cmdline = [
            pythonscad,
            "--trust-python",
            "--enable=predictible-output",
            "--render",
            "-o", cli_dummy_path,
            os.path.abspath(fixture),
        ] + list(extra_args)

        print("export-files run cmdline:", " ".join(cmdline))
        print("export-files run cwd:", str(rundir))
        sys.stdout.flush()
        sys.stderr.flush()

        proc = subprocess.run(
            cmdline,
            cwd=str(rundir),
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    if proc.stdout:
        sys.stderr.write(proc.stdout.decode("utf-8", "replace"))
    if proc.stderr:
        sys.stderr.write(proc.stderr.decode("utf-8", "replace"))
    if proc.returncode != 0:
        print(
            f"Error: pythonscad failed with return code {proc.returncode}",
            file=sys.stderr,
        )
        return False
    return True


# Coverage-instrumentation artifacts emitted by the running binary
# itself when built with ``-DPROFILE=ON`` (see ``CMakeLists.txt``: the
# project sets ``-fprofile-dir=.gcov`` so libgcov writes
# path-mangled ``.gcda`` files into a ``.gcov/`` subdirectory of the
# process's CWD on exit). These are runtime side-effects, not fixture
# outputs, and must not participate in the tree-diff -- otherwise every
# coverage build (Linux CI runs PROFILE=ON) reports the test as having
# produced "unexpected files". The ``.gcno`` notes file is added
# defensively in case a future build flag changes where it lands.
_IGNORED_DIRS = frozenset({".gcov"})
_IGNORED_SUFFIXES = frozenset({".gcda", ".gcno"})


def _is_ignored(rel_parts: tuple, suffix: str) -> bool:
    if suffix.lower() in _IGNORED_SUFFIXES:
        return True
    return any(part in _IGNORED_DIRS for part in rel_parts)


def _discover(directory):
    """Return ``{rel_posix_path: absolute_path}`` for every file under
    ``directory``, recursing through subdirectories. Empty when
    ``directory`` does not exist. Coverage-instrumentation artifacts
    from ``-DPROFILE=ON`` builds (``.gcov/`` subtree plus stray
    ``*.gcda`` / ``*.gcno`` files) are skipped -- see ``_IGNORED_DIRS``
    / ``_IGNORED_SUFFIXES`` above."""
    if not directory.is_dir():
        return {}
    out = {}
    for p in sorted(directory.rglob("*")):
        if not p.is_file():
            continue
        rel = p.relative_to(directory)
        if _is_ignored(rel.parts, p.suffix):
            continue
        out[rel.as_posix()] = p
    return out


def _wipe_dir(directory):
    """Remove everything *inside* ``directory`` but keep the directory
    itself; create it if it does not exist yet. Used to give each run a
    clean rundir without churning the parent ``output/`` tree."""
    if directory.exists():
        for child in directory.iterdir():
            if child.is_dir() and not child.is_symlink():
                shutil.rmtree(child)
            else:
                child.unlink()
    else:
        directory.mkdir(parents=True, exist_ok=True)


def main():
    parser = argparse.ArgumentParser(
        description="Run a PythonSCAD fixture that writes files via in-script "
                    "export() and tree-diff the produced set against goldens.")
    parser.add_argument("--pythonscad", required=True,
                        help="Path to the pythonscad executable.")
    parser.add_argument("--testname", required=True,
                        help="ctest group name; also the regression subdir.")
    parser.add_argument("--basename", required=True,
                        help="Fixture basename (without extension).")
    parser.add_argument(
        "--regressiondir",
        default=os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             "regression"),
        help="Root regression directory (defaults to tests/regression).")
    parser.add_argument("-g", "--generate", action="store_true",
                        help="Generate goldens instead of comparing. Also "
                             "honored when the TEST_GENERATE environment "
                             "variable is set to any non-empty value "
                             "(matches test_cmdline_tool.py).")
    parser.add_argument("fixture", help="Python fixture script to run.")
    parser.add_argument("extra_args", nargs=argparse.REMAINDER,
                        help="Extra arguments forwarded to pythonscad.")
    args = parser.parse_args()

    generate = args.generate or bool(os.getenv("TEST_GENERATE"))

    rundir = Path("output") / args.testname / args.basename
    _wipe_dir(rundir)

    expecteddir = Path(args.regressiondir) / args.testname / args.basename
    if generate:
        expecteddir.mkdir(parents=True, exist_ok=True)

    if not _run_pythonscad(
            args.pythonscad, args.fixture, args.extra_args, rundir):
        return 1

    actual = _discover(rundir)

    if not actual:
        print(
            f"Error: fixture produced no files in {rundir}",
            file=sys.stderr,
        )
        return 1

    if generate:
        # Drop stale goldens that the fixture no longer writes so the
        # expected tree always mirrors the run output.
        for rel, golden in _discover(expecteddir).items():
            if rel not in actual:
                print(f"removing stale golden: {golden}", file=sys.stderr)
                golden.unlink()
        for rel, produced in actual.items():
            _post_process(produced)
            dst = expecteddir / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(str(produced), str(dst))
            print(f"generated golden: {dst}", file=sys.stderr)
        return 0

    expected = _discover(expecteddir)
    if not expected:
        print(
            f"Error: no goldens in {expecteddir}; regenerate with "
            f"TEST_GENERATE=1.",
            file=sys.stderr,
        )
        return 1

    missing = sorted(expected.keys() - actual.keys())
    extra = sorted(actual.keys() - expected.keys())
    ok = True
    if missing:
        print(
            f"Error: fixture failed to produce expected file(s): {missing}",
            file=sys.stderr,
        )
        ok = False
    if extra:
        print(
            f"Error: fixture produced unexpected file(s) without goldens: "
            f"{extra}",
            file=sys.stderr,
        )
        ok = False

    _setup_tct_options()
    for rel in sorted(actual.keys() & expected.keys()):
        produced = actual[rel]
        _post_process(produced)
        tct.expectedfilename = str(expected[rel])
        if not tct.compare_default(str(produced)):
            ok = False

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
