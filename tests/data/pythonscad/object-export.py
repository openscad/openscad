"""Render test: export() (cheatsheet Displaying)."""
from openscad import *

c = cube(5)
c.show()
export(c, "object-export-test.3mf")
