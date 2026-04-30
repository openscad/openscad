"""Regression fixture for the in-script ``export()`` function: Wavefront OBJ.

The ``.obj`` suffix routes through ``fileformat::fromIdentifier()`` to
``FileFormat::OBJ`` in ``python_export_core()``. The driver runs
``post_process_progname`` on the produced file (which rewrites both the
``# PythonSCAD obj exporter`` header line and any ``PythonSCAD Model``
group naming) before the normalized text compare.
"""
from pythonscad import cube, export

export(cube(5), "cube.obj")
