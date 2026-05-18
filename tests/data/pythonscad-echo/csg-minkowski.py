"""Test CSG operations: union via + operator, and minkowski via % operator."""
from openscad import *

# solid + solid -> union (same as |)
a = cube(4) + sphere(r=1, fn=16)
print(a.size)
print(a.position)

# solid % solid -> minkowski; cube(4) % sphere(r=1) grows each dimension by 2*r
b = cube(4) % sphere(r=1, fn=16)
print(b.size)
print(b.position)
