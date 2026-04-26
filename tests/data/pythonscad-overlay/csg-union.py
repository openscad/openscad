"""Render test: union (|) CSG operation via the pythonscad overlay."""
from pythonscad import *

(cube(10) | cube(10).translate([5, 0, 0])).show()
