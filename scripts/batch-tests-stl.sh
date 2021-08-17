#!/bin/sh

cmd="openscad"
[ -x "./openscad" ] && cmd="./openscad"
[ -x "./OpenSCAD.app/Contents/MacOS/OpenSCAD" ] && cmd="./OpenSCAD.app/Contents/MacOS/OpenSCAD"

mkdir -p output
for f in tests/data/scad/*-tests.scad; do
  echo `basename $f .scad`
  "$cmd" -s output/`basename $f .scad`.stl $f
done

