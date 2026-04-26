"""Type stubs for the `openscad` package.

The `openscad` package re-exports `_openscad` 1:1 (plus, in the future,
any pure-Python additions or overrides that should match upstream
OpenSCAD's API). The canonical stubs live in `_openscad`; this module
just re-exports them so editors see the same names regardless of which
package the user imports from.
"""
from _openscad import *  # noqa: F401,F403
from _openscad import (  # noqa: F401
    Color,
    Matrix4x4,
    PyLibFive,
    PyOpenSCAD,
    PyOpenSCADs,
    Vector2,
    Vector3,
)
