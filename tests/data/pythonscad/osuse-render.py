"""Render test: osuse() and showing geometry from an included SCAD module."""
from openscad import *

lib = osuse("../scad/issues/osuse-render-included.scad")
lib.box().show()
