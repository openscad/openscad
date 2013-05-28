#!/bin/bash

rm cascade-*.scad
sleep 0.05
echo "module A() { sphere(5); }" > cascade-A.scad
sleep 0.05
echo "module B() { cube([8,8,8], center=true); }" > cascade-B.scad
sleep 0.05
echo "module C() { cylinder(h=8, r=5, center=true); }" > cascade-C.scad
sleep 0.05
echo "module D() { cylinder(h=10, r1=5, r2=0, center=true); }" > cascade-D.scad
