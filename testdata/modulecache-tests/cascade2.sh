#!/bin/bash

rm cascade-*.scad
sleep 0.1
echo "module A() { sphere(6); }" > cascade-A.scad
sleep 0.1
echo "module B() { cube([10,10,10], center=true); }" > cascade-B.scad
sleep 0.1
echo "module C() { cylinder(h=10, r=6, center=true); }" > cascade-C.scad
sleep 0.1
echo "module D() { cylinder(h=12, r1=6, r2=0, center=true); }" > cascade-D.scad
