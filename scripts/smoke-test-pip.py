#!/usr/bin/env python3
"""Minimal smoke test for the pip-installed openscad module (no CMake/ctest)."""
from openscad import *

c = cube(5)
c.show()
export(c, "/tmp/pip-smoke.3mf")
print("smoke test OK")
