"""Type stubs for the `pythonscad` package.

The `pythonscad` package is a strict superset of `openscad` (which itself
re-exports `_openscad`). PythonSCAD-only additions are surfaced here.
Currently this stub is a 1:1 re-export of `openscad`'s stub.
"""
from openscad import *  # noqa: F401,F403
from openscad import (  # noqa: F401
    Color,
    Matrix4x4,
    PyLibFive,
    PyOpenSCAD,
    PyOpenSCADs,
    Vector2,
    Vector3,
)
