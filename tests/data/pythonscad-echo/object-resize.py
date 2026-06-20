"""Test .resize() accepts 1-3 element dimension vectors (issue #748)."""
from openscad import *

# 2D resize with two dimensions (previously raised TypeError)
s = square(10).resize([20, 30])
print(s.size)

# 1D resize on 3D object
c = cube(10).resize([20])
print(c.size)

# Imported SVG with 2D resize
obj = osimport("../svg/simple.svg").resize([20, 30])
print(obj.size)

# auto=True should propagate to all axes
a = square(10).resize([20, 0], auto=True)
print(a.size)

# auto list form: only Y axis auto-scales
b = cube(10).resize([20, 0, 0], auto=[False, True, False])
print(b.size)

# Scalar newsize applies to all axes
d = cube(10).resize(20)
print(d.size)

# Tuple auto argument
e = square(10).resize([0, 15], auto=(False, True))
print(e.size)
