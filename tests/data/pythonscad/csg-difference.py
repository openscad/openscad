"""Render test: difference (-) CSG operation."""
from openscad import *

(cube(10) - sphere(d=6, fn=24)).show()
