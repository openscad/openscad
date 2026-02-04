"""Render test: polygon() primitive (cheatsheet)."""
from openscad import *

# Triangle; paths=[[0,1,2]] defines one face using points 0,1,2
polygon([[0, 0], [10, 0], [5, 10]], paths=[[0, 1, 2]]).show()
