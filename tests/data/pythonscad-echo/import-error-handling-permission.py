"""Echo test for osimport() and osuse() permission-denied error handling.

Excluded on Windows: os.chmod(0o000) does not prevent the file owner from
reading the file on Windows, so the PermissionError cases cannot be triggered
there. See CMakeLists.txt for the exclusion.

Scenarios covered:
- osimport(): unreadable file      -> PermissionError
- osuse():    unreadable file      -> PermissionError
"""

import os
import stat
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


with tempfile.NamedTemporaryFile(suffix=".stl", delete=False) as f:
    nrf = f.name
try:
    os.chmod(nrf, 0o000)
    expect("osimport unreadable file", lambda: osimport(nrf), PermissionError)
finally:
    os.chmod(nrf, stat.S_IRUSR | stat.S_IWUSR)
    os.unlink(nrf)

with tempfile.NamedTemporaryFile(suffix=".scad", delete=False) as f:
    nrf = f.name
try:
    os.chmod(nrf, 0o000)
    expect("osuse unreadable file", lambda: osuse(nrf), PermissionError)
finally:
    os.chmod(nrf, stat.S_IRUSR | stat.S_IWUSR)
    os.unlink(nrf)
