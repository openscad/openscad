"""Test Minkowski (+) CSG operation: size of cube + sphere."""
from openscad import *

# Small sphere at origin; cube(4) + sphere(r=1) grows each dimension by 2*r
a = cube(4) + sphere(r=1, fn=16)
print(a.size)
print(a.position)
