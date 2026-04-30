"""Regression fixture for the in-script ``export()`` function: 3MF
single-part export.

Hits the single-object branch of ``python_export_core()`` in
``src/python/pyfunctions.cc`` -- the one that wraps the value in a
single ``Export3mfPartInfo`` named ``"OpenSCAD Model"`` before calling
``export_3mf``. The driver normalizes the produced ``.3mf`` via
``post_process_3mf`` (which extracts ``3D/3dmodel.model`` from the ZIP
and scrubs UUIDs / timestamps / lib3mf-version namespace noise) before
the text compare.
"""
from pythonscad import cube, export

export(cube(5), "cube.3mf")
