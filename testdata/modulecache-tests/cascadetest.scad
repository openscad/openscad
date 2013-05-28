include <cascade-A.scad>
include <cascade-B.scad>
use <cascade-C.scad>
use <cascade-D.scad>

A();
translate([11,0,0]) B();
translate([22,0,0]) C();
translate([33,0,0]) D();
