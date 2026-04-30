"""Regression fixture for the in-script ``export()`` function: ASCII STL.

Calls the bare ``export(geom, "<file>.stl")`` form so the test exercises
``python_export()`` (and the underlying ``python_export_core()``) in
``src/python/pyfunctions.cc`` end-to-end. The ``.stl`` suffix resolves
through ``fileformat::fromIdentifier()``'s ``"stl"``-is-an-alias-for-
``"asciistl"`` path, so the produced file is ASCII STL.

The driver runs PythonSCAD with the process CWD set to the per-test
scratch directory, so the relative ``"cube.stl"`` lands where the
driver expects it; the driver then runs ``post_process_progname`` on
the file before the text compare. Geometry is intentionally tiny and
axis-aligned so the golden stays compact and stable under
``--enable=predictible-output``.
"""
from pythonscad import cube, export

export(cube(5), "cube.stl")
