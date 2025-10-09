from openscad import *
fn=32
a = cylinder(h=20, d=6).translate([10, 10, 10])
print(a.bbox.size)      # [6.0, 6.0, 20.0]
print(a.bbox.position)  # [7.0, 7.0, 10.0]
