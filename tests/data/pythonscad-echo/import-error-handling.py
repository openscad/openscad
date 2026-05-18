"""Echo test for osimport() and osuse() error handling (cross-platform cases).

Scenarios covered:
- osimport(): empty filename       -> ValueError
- osimport(): nonexistent file     -> FileNotFoundError
- osimport(): path is a directory  -> OSError
- osuse():    empty filename       -> ValueError
- osuse():    nonexistent file     -> FileNotFoundError
- osuse():    path is a directory  -> OSError

Permission-denied cases are in import-error-handling-permission.py,
which is excluded on Windows (chmod semantics differ).
"""

import tempfile

from openscad import osimport, osuse


def expect(label, fn, exc):
    try:
        fn()
    except exc as e:
        print(f"{label}: {type(e).__name__}")
    except Exception as e:
        print(f"{label}: UNEXPECTED {type(e).__name__}: {e}")
    else:
        print(f"{label}: NO EXCEPTION (expected {exc.__name__})")


# --- osimport() ---
expect("osimport empty filename", lambda: osimport(""), ValueError)
expect(
    "osimport nonexistent file",
    lambda: osimport("__nonexistent_file_that_cannot_exist_xyz__.stl"),
    FileNotFoundError,
)
with tempfile.TemporaryDirectory() as tmp:
    expect("osimport directory path", lambda: osimport(tmp), OSError)

# --- osuse() ---
expect("osuse empty filename", lambda: osuse(""), ValueError)
expect(
    "osuse nonexistent file",
    lambda: osuse("__nonexistent_file_that_cannot_exist_xyz__.scad"),
    FileNotFoundError,
)
with tempfile.TemporaryDirectory() as tmp:
    expect("osuse directory path", lambda: osuse(tmp), OSError)
