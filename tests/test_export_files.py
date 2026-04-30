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
#      STL/STLBIN/SVG/OBJ/POV, inner-XML extraction for 3MF; other
#      extensions pass through untouched) keyed by each file's own
#      extension, then compares each produced file against
#      `tests/regression/<testname>/<basename>/<rel-path>`.
#      Suffixes listed in ``_BINARY_SUFFIXES`` (currently the binary-STL
#      fallthrough suffix ``.stlbin``) take a strict
#      ``filecmp.cmp(shallow=False)`` bytes-equality path because
#      text-line normalization would corrupt the embedded floats / int
#      packing; everything else uses ``test_cmdline_tool.compare_default()``
#      -- a normalized text comparison (line-ending normalization +
#      unified diff). For the ASCII / text-derived formats that the
#      post-processors normalize, the text compare is effectively
#      bytes-equality after normalization. The expected directory
#      mirrors the actual directory layout one-for-one (relative path
#      is the key on both sides), so missing or unexpected files are
#      caught by simple set diffing.
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
import filecmp
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


# Length of the fixed-size text header at the start of a binary STL
# file (see ``src/io/export_stl.cc``: ``char header[80] = ...``); the
# first 4 bytes after that are the little-endian uint32 triangle count
# and after that are per-triangle 50-byte records.
_BINARY_STL_HEADER_LEN = 80


def post_process_stlbin(filename):
    """Length-preserving rewrite of the 80-byte binary STL header.

    Reusing ``tct.post_process_progname`` directly on a binary STL is
    unsafe: of its five ``bytes.replace`` substitutions, only the
    first two (``PythonSCAD Model\\x0a\\x00`` -> ``OpenSCAD Model
    \\x0a\\x00\\x00\\x00`` and the ``_Model`` variant) are
    length-preserving. The remaining three (``PythonSCAD_Model`` ->
    ``OpenSCAD_Model``, etc.) shrink by two bytes; if they ever fired
    on a binary STL -- whether because the writer changed how it
    terminates the model name in the 80-byte header, or because the
    raw triangle bytes coincidentally spelled out one of those
    patterns -- the file would lose two bytes and every subsequent
    triangle-count / triangle-record offset would shift.

    Sidestep that by only touching the fixed 80-byte header and only
    applying the length-preserving padding-compensated substitution
    that the binary writer's header layout actually produces. The
    triangle data starting at byte 80 is left strictly untouched.
    Length is asserted to never change so the rewrite cannot silently
    desynchronize the body from the header.
    """
    with open(filename, "rb") as f:
        content = f.read()
    if len(content) < _BINARY_STL_HEADER_LEN:
        # Truncated / malformed binary STL: leave it alone so the
        # comparison step surfaces the real problem instead of having
        # this normalizer mask it.
        return
    header = content[:_BINARY_STL_HEADER_LEN]
    body = content[_BINARY_STL_HEADER_LEN:]
    new_header = header.replace(
        b"PythonSCAD Model\x0a\x00",
        b"OpenSCAD Model\x0a\x00\x00\x00",
    )
    if len(new_header) != _BINARY_STL_HEADER_LEN:
        raise RuntimeError(
            f"post_process_stlbin: header length changed from "
            f"{_BINARY_STL_HEADER_LEN} to {len(new_header)} "
            f"-- binary STL branding layout drifted?"
        )
    with open(filename, "wb") as f:
        f.write(new_header + body)


# Format-aware post-processors, keyed by lowercase file extension
# (including the leading dot). Files whose extension is not listed pass
# through untouched and are compared as-is by ``tct.compare_default``,
# which already does line-ending-tolerant text comparison. Add a new
# entry here if a future format requires extra normalization.
#
# ``.stlbin`` is the conventional fallthrough suffix used by fixtures
# that want to exercise the binary-STL code path through the in-script
# ``export()`` function: the suffix is unrecognized by
# ``fileformat::fromIdentifier``, so ``python_export_core`` keeps its
# ``BINARY_STL`` default (see ``src/python/pyfunctions.cc``). The
# resulting file is real binary STL whose 80-byte header carries the
# PythonSCAD branding, so we route it through the dedicated
# ``post_process_stlbin`` helper above (which only rewrites the fixed
# header and never touches the triangle data) rather than reusing
# ``tct.post_process_progname`` -- see that helper's docstring for
# why generic, partially length-changing text replacements are not
# safe on binary STL bytes.
#
# AMF is intentionally *not* listed here: AMF mesh tessellation
# (vertex order + triangle indexing) is not stable across the CI
# matrix even with --enable=predictible-output, so byte-compare is not
# viable yet. See PR #590 / issue #586 and the corresponding note in
# ``tests/CMakeLists.txt``. If a future change makes AMF deterministic,
# a ``post_process_amf`` helper that normalizes the
# ``<metadata type="producer">PythonSCAD <version></metadata>`` line
# (currently emitted from ``src/io/export_amf.cc``) will need to be
# reintroduced alongside the fixture.
_POST_PROCESSORS = {
    ".stl": tct.post_process_progname,
    ".stlbin": post_process_stlbin,
    ".svg": tct.post_process_progname,
    ".obj": tct.post_process_progname,
    ".3mf": tct.post_process_3mf,
    ".pov": tct.post_process_progname,
}


# Suffixes whose produced files are genuinely binary -- text-line
# normalization in ``tct.compare_default`` would corrupt embedded
# floats / int packing, so these go through a strict bytes-equality
# check instead. Today this is only the binary-STL fallthrough suffix
# (see ``_POST_PROCESSORS`` note above); add new entries here when a
# new binary format gets fixture coverage.
_BINARY_SUFFIXES = frozenset({".stlbin"})


def _post_process(path):
    """Normalize ``path`` in place based on its extension, if known."""
    fn = _POST_PROCESSORS.get(Path(path).suffix.lower())
    if fn is not None:
        fn(str(path))


def _is_binary(path):
    return Path(path).suffix.lower() in _BINARY_SUFFIXES


def _compare_bytes(expected, actual):
    """Strict bytes-equality compare with a small diagnostic on mismatch.

    Returns True on match, False otherwise. Used for files in
    ``_BINARY_SUFFIXES`` where the existing text-mode
    ``tct.compare_default`` would mangle non-text bytes. Stays silent
    on the happy path so passing ctests do not get noisy headers; only
    emits diagnostic lines when ``filecmp.cmp`` reports a mismatch.
    """
    if filecmp.cmp(str(expected), str(actual), shallow=False):
        return True
    print('binary comparison: ', file=sys.stderr)
    print(' expected file: ', expected, file=sys.stderr)
    print(' actual file:   ', actual, file=sys.stderr)
    expected_bytes = Path(expected).read_bytes()
    actual_bytes = Path(actual).read_bytes()
    if len(expected_bytes) != len(actual_bytes):
        print(
            f' size mismatch: expected={len(expected_bytes)} '
            f'actual={len(actual_bytes)}',
            file=sys.stderr,
        )
    common_prefix = min(len(expected_bytes), len(actual_bytes))
    first_diff = None
    for i in range(common_prefix):
        eb = expected_bytes[i]
        ab = actual_bytes[i]
        if eb != ab:
            first_diff = (
                f' first byte diff at offset {i}: '
                f'expected={eb:#04x} actual={ab:#04x}'
            )
            break
    # Files matched up to the shorter length but differed in size --
    # one is a strict prefix of the other, so the first differing
    # offset is at the boundary itself; report the truncated side as
    # ``<eof>`` so the caller sees where divergence really is.
    if first_diff is None and len(expected_bytes) != len(actual_bytes):
        i = common_prefix
        if len(expected_bytes) < len(actual_bytes):
            first_diff = (
                f' first byte diff at offset {i}: '
                f'expected=<eof> actual={actual_bytes[i]:#04x}'
            )
        else:
            first_diff = (
                f' first byte diff at offset {i}: '
                f'expected={expected_bytes[i]:#04x} actual=<eof>'
            )
    if first_diff is not None:
        print(first_diff, file=sys.stderr)
    return False


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
        if _is_binary(produced):
            if not _compare_bytes(expected[rel], produced):
                ok = False
        else:
            tct.expectedfilename = str(expected[rel])
            if not tct.compare_default(str(produced)):
                ok = False

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
