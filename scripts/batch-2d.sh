#!/bin/sh

cmd="openscad"
[ -x "./openscad" ] && cmd="./openscad"
[ -x "./OpenSCAD.app/Contents/MacOS/OpenSCAD" ] && cmd="./OpenSCAD.app/Contents/MacOS/OpenSCAD"

mkdir -p output
for f in tests/data/dxf/*.dxf; do
  echo `basename $f`
  cat > tmp.scad << EOF
  import_dxf("$f");
EOF
  "$cmd" -x output/`basename $f` tmp.scad
done

