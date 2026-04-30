"""Regression fixture for the in-script ``export()`` function: 3MF
multi-part / multi-color export via the dict form.

Hits the ``PyDict_Check(obj)`` branch of ``python_export_core()`` in
``src/python/pyfunctions.cc``: for every key/value pair the dict
contributes one ``Export3mfPartInfo`` named after the dict key, so the
resulting 3MF carries two named, distinctly-colored parts. The driver
normalizes via ``post_process_3mf`` (XML extraction + UUID / timestamp
/ namespace scrub) before the text compare.

Geometry is two non-overlapping boxes with explicit ``.color()`` calls
so the per-part base material survives the round trip into the 3MF
material registry.
"""
from pythonscad import cube, export

red_part = cube([6, 6, 4]).color("red")
blue_part = cube([6, 6, 4]).translate([8, 0, 0]).color("blue")

export({"red": red_part, "blue": blue_part}, "two-color.3mf")
