from pythonscad import *

# The cylinder overlaps the clearance needed by the masked base fillet. This
# used to abort in createEdgeDb(); it should now fail with a diagnostic.
x = 2.5
y = 3.5

solids = union([
    cube([80, 47, 3]),
    cylinder(d=7.5, h=5) + [x, y, 3],
])
remove = union(
    cylinder(d=2.5, h=100) + [x, y, 0],
    cylinder(d=3.7, h=100) + [x, y, 7.4],
)
tmp = difference(solids, remove)
mask = cube([30, 1, 30], center=True)
tmp.fillet(3, mask, fn=20).show()
