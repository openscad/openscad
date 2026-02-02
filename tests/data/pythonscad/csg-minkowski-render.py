"""Render test: Minkowski (+) CSG operation."""
from openscad import *

(cube(6) + sphere(r=1.5, fn=16)).show()
