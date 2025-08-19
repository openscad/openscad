from openscad import *
a = cube(10)
b = cube(10).translate([5, 5, 5])

print(a.size)         # [10.0, 10.0, 10.0]
print(b.size)         # [10.0, 10.0, 10.0]
print((a | b).size)   # [15.0, 15.0, 15.0]
print((a - b).size)   # [10.0, 10.0, 10.0]
print((a & b).size)   # [5.0, 5.0, 5.0]
