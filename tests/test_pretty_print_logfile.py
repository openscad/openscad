#!/usr/bin/env python3

"""findlogfile() must pick the log ctest just wrote, not whichever one the glob yields first.

An interrupted ctest run leaves a LastTest.log.tmp<random> behind. findlogfile() globs
'LastTest.log*'. Filesystem-order-first picked a stale temp file and either reported an old run's
results or failed outright on an output type it held. Newest-by-mtime fixes the single-run case,
but two concurrent ctest invocations against the same builddir can each leave a fresh-looking file,
and mtime alone can't tell which one belongs to *this* invocation.

So CTestCustom.template's CTEST_CUSTOM_PRE_TEST hook touches a marker file right before ctest
starts running tests. findlogfile() then only considers candidates written at or after that marker
-- narrowing "the log this run wrote" from "whichever file happens to be newest" to "whichever file
appeared after this run began". A run with no fresh candidate reports a clear error instead of
silently reporting a stale run's results.
"""

import importlib.util
import os
import sys
import tempfile
from pathlib import Path

module_path = Path(__file__).with_name("test_pretty_print.py")
spec = importlib.util.spec_from_file_location("pretty_print_under_test", module_path)
pretty_print = importlib.util.module_from_spec(spec)
sys.argv = [str(module_path)]  # the module parses sys.argv at import time
spec.loader.exec_module(pretty_print)


def write(path, mtime):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("log\n")
    os.utime(path, (mtime, mtime))


def check(name, files, marker_mtime, expected):
    with tempfile.TemporaryDirectory() as directory:
        for filename, mtime in files:
            write(Path(directory) / "Testing" / "Temporary" / filename, mtime)
        if marker_mtime is not None:
            write(Path(directory) / "Testing" / "Temporary" / pretty_print.RUN_MARKER_NAME,
                  marker_mtime)
        chosen = Path(pretty_print.findlogfile(directory)).name
        assert chosen == expected, f"{name}: picked {chosen}, expected {expected}"
        print(f"{name}: {chosen}")


def check_no_fresh_log(name, files, marker_mtime):
    with tempfile.TemporaryDirectory() as directory:
        for filename, mtime in files:
            write(Path(directory) / "Testing" / "Temporary" / filename, mtime)
        write(Path(directory) / "Testing" / "Temporary" / pretty_print.RUN_MARKER_NAME,
              marker_mtime)
        try:
            pretty_print.findlogfile(directory)
        except SystemExit:
            print(f"{name}: exited as expected")
            return
        raise AssertionError(f"{name}: expected findlogfile to exit, but it returned a path")


def main():
    # A finished run, no concurrent invocation: the real log is newest and postdates the marker.
    check("finished run", [("LastTest.log", 2000), ("LastTest.log.tmp2fa58", 1000)], 1500,
          "LastTest.log")
    # A run still in progress: ctest has not flushed LastTest.log yet, so the temp file is current.
    check("run in progress", [("LastTest.log", 1000), ("LastTest.log.tmpabcde", 2000)], 1500,
          "LastTest.log.tmpabcde")
    # A stale temp file from days ago predates this run's marker and must be excluded even though
    # it's the only other candidate.
    check("stale temp excluded by marker",
          [("LastTest.log", 3000), ("LastTest.log.tmpaaaaa", 1000)], 2000, "LastTest.log")
    # No marker on disk (script run directly, outside the wired-up ctest hook): falls back to
    # newest-by-mtime, same as before the marker existed.
    check("no marker falls back to newest",
          [("LastTest.log", 2000), ("LastTest.log.tmp2fa58", 1000)], None, "LastTest.log")
    # Nothing postdates the marker: ctest was interrupted before writing anything for this run.
    # Reporting the stale log would silently describe an old run, so this must fail loudly instead.
    check_no_fresh_log("nothing fresh since marker",
                        [("LastTest.log", 1000), ("LastTest.log.tmpaaaaa", 1200)], 2000)


if __name__ == "__main__":
    main()
