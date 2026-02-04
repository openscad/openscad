"""Render test: show(list of objects) (cheatsheet Python-Specific)."""
from openscad import *

show([cube(4).right(5 * i) for i in range(5)])
