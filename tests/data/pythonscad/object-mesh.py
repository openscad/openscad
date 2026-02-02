"""Render test: .mesh() and polyhedron(pts, tri) (cheatsheet Python-Specific)."""
from openscad import *

c = cube(5)
pts, tri = c.mesh()
# Modify mesh: push vertices with z>3 and y>3 upward
for pt in pts:
    if pt[2] > 3 and pt[1] > 3:
        pt[2] = pt[2] + 3
polyhedron(pts, tri).show()
