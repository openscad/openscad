"""Regression fixture for ``MultiToolExporter.export(single_file=...)``.

This exercises the helper's single-file 3MF path, which computes named
parts via ``MultiToolExporter.parts()`` and forwards them to PythonSCAD's
dict-based multi-object 3MF export.
"""
from pythonscad import MultiToolExporter, cube

red_part = cube([6, 6, 4]).color("red")
blue_part = cube([6, 6, 4]).translate([8, 0, 0]).color("blue")

MultiToolExporter("", ".stl", items=[
    ("red", red_part),
    ("blue", blue_part),
]).export(single_file="multitool.3mf")
