"""Render test: rounded_cube() via the pythonscad overlay package."""
from pythonscad import *

rounded_cube(20, r=2, fn=100).show()
rounded_cube([30, 20, 10], d=4, fn=50).translate([22, 0, 0]).show()
