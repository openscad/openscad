from openscad import *
fn=32

a = cube(10)
b = cube(10).translate([5, 5, 5])
c = cylinder(h=20, d=6)

print(a.size)         # [10.0, 10.0, 10.0]
print(b.size)         # [10.0, 10.0, 10.0]
print((a | b).size)   # [15.0, 15.0, 15.0]
print((a - b).size)   # [10.0, 10.0, 10.0]
print((a & b).size)   # [5.0, 5.0, 5.0]
print(c.size)         # [6.0, 6.0, 20.0]

# 2D objects
s = square([10, 20])
t = square([10, 20]).translate([5, 5])
u = circle(10)

print(s.size)         # [10.0, 20.0]
print(t.size)         # [10.0, 20.0]
print((s | t).size)   # [15.0, 25.0]
print(u.size)         # [20.0, 20.0]
