"""Test .rotate() object manipulation: bbox size after rotation."""
from openscad import *

c = cube(10).rotate([0, 0, 45])
print(c.size)
print(c.position)
