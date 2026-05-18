"""Echo test for export() error handling.

Verifies that export() raises OSError when the target file cannot be written,
rather than silently succeeding.

Scenarios covered:
- Parent directory does not exist  -> OSError
- Empty filename                   -> OSError or ValueError

Scenarios NOT covered here (hard to simulate in a portable test):
- Disk full: would require filling the filesystem
- Insufficient permissions: platform-dependent and unsafe to set up in CI
- File locked (Windows): not applicable on Linux/macOS
"""

import os
import tempfile

from openscad import cube, export


def expect(label, fn, exc):
    try:
        fn()
    except exc as e:
        print(f"{label}: {type(e).__name__}")
    except Exception as e:
        print(f"{label}: UNEXPECTED {type(e).__name__}: {e}")
    else:
        print(f"{label}: NO EXCEPTION (expected {exc.__name__})")


obj = cube(10)

# Parent directory does not exist
expect(
    "nonexistent parent dir",
    lambda: export(obj, "/__nonexistent_dir_xyz__/out.stl"),
    OSError,
)

# Empty filename
expect("empty filename", lambda: export(obj, ""), OSError)
