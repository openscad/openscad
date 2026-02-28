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

# 2D objects
s = square([10, 20])
t = square([10, 20]).translate([5, 5])
u = circle(10)

print(s.position)         # [0.0, 0.0]
print(t.position)         # [5.0, 5.0]
print((s | t).position)   # [0.0, 0.0]
print(u.position)         # [-10.0, -10.0]
