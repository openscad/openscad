"""Test .scale() object manipulation: size after scaling."""
from openscad import *

c = cube(10).scale([2, 1, 1])
print(c.size)
print(c.position)
