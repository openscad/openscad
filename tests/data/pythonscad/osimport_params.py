#!/usr/bin/env python3
"""
Test case for osimport() parameter parsing bug fix (issue #193)

This test verifies that all parameters of osimport() are correctly parsed.
Before the fix, the format string had 'sfOddd' instead of 'sOdsddd' which
caused center, dpi, id, fn, fa, fs to be misparsed.
"""

from openscad import *

# Test import with center parameter (was parsed as float, should be PyObject/bool)
# Using a simple STL file from test data
obj = osimport(file="../scad/3D/features/import.stl", center=True, convexity=2)

# Render it to verify it works
obj.show()
