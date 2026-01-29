"""Regression test for segfault when None is returned from union operation.

This tests the fix for two related bugs:
1. PyOpenSCADObjectToNode was casting Py_None to PyOpenSCADObject* and accessing
   ->node without type checking, causing a segfault
2. python_nb_sub was returning arg1/arg2 without incrementing reference counts,
   causing use-after-free

The bug triggers when a union operation returns Py_None (which is valid for
`cube(10) | None`), but then that None is passed back to the union operator
in a second union (`cube(10) | None | None`). The second union tries to process
the returned None object, causing the segfault.
"""
from openscad import *

show(cube(10) | None | None)
