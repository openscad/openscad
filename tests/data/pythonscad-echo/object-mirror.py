"""Test .mirror() object manipulation: position after mirror."""
from openscad import *

c = cube(10).mirror([1, 0, 0])
print(c.size)
print(c.position)
