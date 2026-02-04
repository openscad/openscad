"""Render test: chained object manipulation (color, CSG, rotate)."""
from openscad import *

(cube(10) - sphere(d=5, fn=24)).color("Tomato").rotate([0, 0, 30]).show()
