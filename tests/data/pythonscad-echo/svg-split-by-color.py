"""Echo test for ``osimport(..., split_by_color=True)``.

Exercises:
  * split_by_color=True on a multi-color SVG returns a dict keyed by
    hex color, one entry per distinct selected outline color
  * default (no flag) and explicit split_by_color=False both keep
    returning a single PyOpenSCAD object (backward compatibility)
  * split_by_color=True on a non-SVG import raises ValueError
"""

from pythonscad import osimport

SVG = "../svg/box-w-holes.svg"
STL = "../scad/3D/features/import.stl"

parts = osimport(SVG, split_by_color=True)
print("type:", type(parts).__name__)
print("keys:", sorted(parts.keys()))
print("all values are PyOpenSCAD:", all(type(v).__name__ == "PyOpenSCAD" for v in parts.values()))

truthy_result = osimport(SVG, layer=None, id=None, split_by_color=1)
print("None selectors and truthy flag return dict:", isinstance(truthy_result, dict))

positional_result = osimport(SVG, None, 2, None, 1, 1, 1, False, 72, None, False, None, None, None, True)
print("fully positional flag returns dict:", isinstance(positional_result, dict))

try:
    osimport(SVG, origin=["invalid", 0])
    print("invalid origin: NO EXCEPTION (expected TypeError)")
except TypeError:
    print("invalid origin: TypeError")

default_result = osimport(SVG)
print("default is dict:", isinstance(default_result, dict))
print("default type:", type(default_result).__name__)

explicit_false_result = osimport(SVG, split_by_color=False)
print("explicit False is dict:", isinstance(explicit_false_result, dict))
print("explicit False type:", type(explicit_false_result).__name__)

try:
    osimport(STL, split_by_color=True)
    print("non-svg split_by_color: NO EXCEPTION (expected ValueError)")
except ValueError as e:
    print("non-svg split_by_color: ValueError")
