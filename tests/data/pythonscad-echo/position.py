from openscad import *
fn=32

a = cube(10)
b = cube(10).translate([5, 5, 5])
c = cylinder(h=20, d=6)

print(a.position)         # [0.0, 0.0, 0.0]
print(b.position)         # [5.0, 5.0, 5.0]
print((a | b).position)   # [0.0, 0.0, 0.0]
print((a - b).position)   # [0.0, 0.0, 0.0]
print((a & b).position)   # [5.0, 5.0, 5.0]
print(c.position)         # [-3.0, -3.0, 0.0]
