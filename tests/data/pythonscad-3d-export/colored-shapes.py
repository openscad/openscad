"""3D export test: colored union of cube and sphere."""
from openscad import *

c = cube([10, 10, 10]).color("blue")
s = sphere(r=7, fn=32).translate([15, 0, 0]).color("green")
(c | s).show()
