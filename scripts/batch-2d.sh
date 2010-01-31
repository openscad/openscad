#!/bin/sh

mkdir -p output
for f in testdata/*.dxf; do
  echo `basename $f`
  cat > tmp.scad << EOF
  import_dxf("$f");
EOF
  ./OpenSCAD.app/Contents/MacOS/OpenSCAD -x output/`basename $f` tmp.scad
done