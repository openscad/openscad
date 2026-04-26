"""PythonSCAD public API.

Strict superset of :mod:`openscad`. Currently a 1:1 re-export; future
PythonSCAD-only additions (new classes, helper APIs) will land here as
either symbols in this package or as submodules.

This package re-exports :mod:`openscad` rather than :mod:`_openscad`
directly, so any pure-Python implementation or override added in
:mod:`openscad` is automatically picked up.

The three-module layout is:

* :mod:`_openscad`  - C extension (low level, implementation detail).
* :mod:`openscad`   - ``_openscad`` + OpenSCAD-compatible pure-Python
  additions/overrides.
* :mod:`pythonscad` - this module: ``openscad`` + PythonSCAD-only
  extensions.

Switching a script between ``from openscad import *`` and
``from pythonscad import *`` requires no other code changes.
"""

from openscad import *  # noqa: F401,F403
from openscad import (  # noqa: F401
    ChildIterator,
    ChildRef,
    Openscad,
)
