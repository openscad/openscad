#! ${PYTHON_EXECUTABLE}
# # This python script generates a large output .scad file (3.1MB) for stress testing the parser.
# See Issue #2342 / Pull Request #2343

from __future__ import print_function

xcount = 100
ycount = 100
zcount = 10
totalcount = xcount*ycount*zcount

for x in range(1, xcount):
    for y in range(1, ycount):
        for z in range(1, zcount):
            print("translate([{}, {}, {}])".format(x,y,z))
            print("  cube(0.5);")

print("echo(\"{} elements processed\");".format(totalcount))
