##########################
# PythonSCAD Logo        #
##########################
# inspired by python logo
##########################

from pythonscad import *

fn = 80
body = cube(12, center=True) % sphere(r=4)
mask = hull(cylinder(r=3, h=40).rotx(90) ^ [[2.5, 30], 20, [0, -20]])

part1 = body - ((cube([10, 20, 2]) + [-10, -10, -3]))
part1 -= cylinder(r=2, h=20).rotx(90) + [-4, 10, 5]

part2 = body - mask
final = part1.up(10.5) | (part2 + [-10, 0, -2.5])

final.color("#346e9d").show()
final.rotx(180).mirror([1, 0, 0]).color("#ffe468").show()
