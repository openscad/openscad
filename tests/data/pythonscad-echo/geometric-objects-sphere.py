"""Test sphere() geometric object: size and position properties."""
from openscad import *

s1 = sphere(d=10, fn=32)
s2 = sphere(r=5, fn=32)
print(s1.size)
print(s1.position)
print(s2.size)
print(s2.position)
