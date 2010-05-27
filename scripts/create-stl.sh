#!/bin/sh

if [ $# != 2 ]; then
  echo "Usage: $0 scad-file stl-file"
  exit 1
fi
scadfile=$1
stlfile=$2

cmd="openscad"
[ -x "./openscad" ] && cmd="./openscad"
[ -x "./OpenSCAD.app/Contents/MacOS/OpenSCAD" ] && cmd="./OpenSCAD.app/Contents/MacOS/OpenSCAD"

"$cmd" -s $stlfile $scadfile
