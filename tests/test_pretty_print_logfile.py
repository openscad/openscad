#!/usr/bin/env python3

"""findlogfile() must pick the log ctest just wrote, not whichever one the glob yields first.

An interrupted ctest run leaves a LastTest.log.tmp<random> behind. findlogfile() globs
'LastTest.log*' and took the first match in filesystem order, so a single stale temp file made
every later run's report describe that old run instead -- reporting tests that had long since been
fixed, and failing the run outright if the stale log held an output type the report cannot render.
The temp file still has to win while a run is genuinely in progress, which is why this picks the
newest rather than preferring one name.
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


def check(name, files, expected):
    with tempfile.TemporaryDirectory() as directory:
        for filename, mtime in files:
            write(Path(directory) / "Testing" / "Temporary" / filename, mtime)
        chosen = Path(pretty_print.findlogfile(directory)).name
        assert chosen == expected, f"{name}: picked {chosen}, expected {expected}"
        print(f"{name}: {chosen}")


def main():
    # A finished run: the real log is newest and is the one describing this run.
    check("finished run", [("LastTest.log", 2000), ("LastTest.log.tmp2fa58", 1000)], "LastTest.log")
    # A run still in progress: ctest has not flushed LastTest.log yet, so the temp file is current.
    check("run in progress", [("LastTest.log", 1000), ("LastTest.log.tmpabcde", 2000)],
          "LastTest.log.tmpabcde")
    # Several stale temp files from several interrupted runs.
    check("many stale temps",
          [("LastTest.log", 3000), ("LastTest.log.tmpaaaaa", 1000), ("LastTest.log.tmpzzzzz", 2000)],
          "LastTest.log")


if __name__ == "__main__":
    main()
