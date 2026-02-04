"""Test osuse() variable access: read assignment from included SCAD file."""
from openscad import *

lib = osuse("../scad/issues/issue386-pythonscad-included.scad")
print(lib["$bar"])
print(lib.test())
