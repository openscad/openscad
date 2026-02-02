"""Render test: hull() (cheatsheet Advanced)."""
from openscad import *

# Convex hull of two spheres
hull(sphere(3, fn=24), sphere(3, fn=24).translate([6, 0, 0])).show()
