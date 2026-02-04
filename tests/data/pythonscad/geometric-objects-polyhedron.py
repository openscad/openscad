"""Render test: polyhedron() primitive (cheatsheet)."""
from openscad import *

# Tetrahedron: 4 points, 4 triangular faces
points = [[0, 0, 0], [10, 0, 0], [5, 10, 0], [5, 5, 10]]
faces = [[0, 1, 2], [0, 1, 3], [0, 2, 3], [1, 2, 3]]
polyhedron(points, faces).show()
