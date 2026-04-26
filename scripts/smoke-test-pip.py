#!/usr/bin/env python3
"""Minimal smoke test for the pip-installed PythonSCAD module (no CMake/ctest).

Exercises the three-module layout shipped by the wheel:

* ``_openscad``  - C extension (low level).
* ``openscad``   - pure-Python overlay re-exporting ``_openscad`` (kept
  for compatibility with upstream OpenSCAD).
* ``pythonscad`` - strict superset of ``openscad``, the recommended
  import for PythonSCAD-only code.

Asserts the drop-in property: switching between ``from openscad import *``
and ``from pythonscad import *`` does not change which callables a script
binds (they are the same objects).
"""
import os
import tempfile

import _openscad
import openscad
import pythonscad

assert openscad.cube is _openscad.cube, (
    "openscad.cube should be the same object as _openscad.cube"
)
assert pythonscad.cube is openscad.cube, (
    "pythonscad.cube should be the same object as openscad.cube"
)

assert set(dir(openscad)) >= set(n for n in dir(_openscad) if not n.startswith("_")), (
    "openscad must re-export every public name from _openscad"
)
assert set(dir(pythonscad)) >= set(n for n in dir(openscad) if not n.startswith("_")), (
    "pythonscad must re-export every public name from openscad"
)

# Hold an explicit reference to the TemporaryDirectory so it stays alive
# until the script exits; the directory (and therefore the export
# artifacts) is cleaned up automatically.  Using mkdtemp-style isolation
# keeps the smoke test safe to run in parallel and avoids leaving stale
# files behind in /tmp.
_workdir = tempfile.TemporaryDirectory(prefix="pythonscad-pip-smoke-")
WORKDIR = _workdir.name

from openscad import *  # noqa: F401,F403,E402

c = cube(5)
c.show()
export(c, os.path.join(WORKDIR, "pip-smoke-openscad.3mf"))

from pythonscad import *  # noqa: F401,F403,E402

c2 = cube(5)
c2.show()
export(c2, os.path.join(WORKDIR, "pip-smoke-pythonscad.3mf"))

print("smoke test OK")
