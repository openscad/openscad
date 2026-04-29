"""MultiToolExporter regression fixture: two overlapping bricks split into
red and blue parts.

The test driver (``tests/test_export_files.py``) runs PythonSCAD with the
*process CWD* set to a per-test scratch run directory, so the exporter's
relative filenames (``"red.stl"``, ``"blue.stl"``) land where the driver
expects them. The driver then post-processes each produced file
(``post_process_progname`` for STL/SVG/OBJ, inner-XML extraction for 3MF)
and compares it against a checked-in golden using
``test_cmdline_tool.compare_default()`` -- a normalized text comparison
(line-ending normalization + unified diff), not a raw byte-for-byte
comparison.

Geometry is intentionally small and axis-aligned so the resulting ASCII
STL stays compact and stable under ``--enable=predictible-output``.
"""
from pythonscad import MultiToolExporter, cube

background = cube([20, 20, 4])
star = cube([8, 8, 4]).translate([6, 6, 0])

MultiToolExporter("", ".stl", items=[
    ("blue", background),
    ("red", star),
]).export()
