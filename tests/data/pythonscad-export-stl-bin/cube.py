"""Regression fixture for the in-script ``export()`` function: binary STL
via the unrecognized-suffix fallthrough.

``python_export_core()`` in ``src/python/pyfunctions.cc`` initializes
``exportFileFormat`` to ``FileFormat::BINARY_STL`` and only overrides
it when the file's suffix matches a registered identifier. Today
``export()`` has no format kwarg, so the only way to exercise the
binary-STL writer through the Python function is to pick a suffix
that ``fileformat::fromIdentifier()`` does not recognize -- here
``.stlbin`` -- so the default ``BINARY_STL`` survives. The C++ side
will print a "Invalid suffix stlbin. Defaulting to binary STL" log
line; that is informational, not a failure.

The matching driver entry in ``tests/test_export_files.py`` registers
``.stlbin`` in both ``_POST_PROCESSORS`` (for the 80-byte header
``PythonSCAD Model`` -> ``OpenSCAD Model`` rewrite) and in
``_BINARY_SUFFIXES`` (so the comparison drops to strict
``filecmp.cmp(shallow=False)``, since text-mode line normalization
would corrupt the embedded float / triangle bytes).
"""
from pythonscad import cube, export

export(cube(5), "cube.stlbin")
