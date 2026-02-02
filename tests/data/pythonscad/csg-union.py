"""Render test: union (|) CSG operation."""
from openscad import *

(cube(10) | cube(10).translate([5, 0, 0])).show()
