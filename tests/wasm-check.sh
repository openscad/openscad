#!/usr/bin/env bash
#
# Does a light check of a node or web WASM build.
#
set -euo pipefail

cd $(dirname $0)

ENV=$1

rm -f out.stl
case $ENV in
  node)
    echo "Checking WASM node build..."
    n use latest
    chmod +x ../../build/openscad.js
    ../../build/openscad.js \
        --backend=manifold \
        $PWD/../examples/Basics/CSG.scad \
        -o out.stl
    cat out.stl
    ;;
  node-module)
    echo "Checking WASM node module build..."
    n use latest
    node -e "import OpenSCAD from '../../build/openscad.js';\
      OpenSCAD({noInitialRun: true}).then(instance => instance.callMain([\
        '$PWD/../examples/Basics/CSG.scad',\
        '-o', 'out.stl',\
        '--backend=manifold'\
      ]))"
    cat out.stl
    ;;
  web)
    echo "Checking WASM web build..."
    node wasm-check.mjs $PWD/wasm-check.html
    ;;
  *)
    echo "Usage: $0 {node|web|node-module}"
    exit 1
    ;;
esac 2>&1 | tee out.log
cat out.log | grep "Top level object is a 3D object (manifold)" >/dev/null
echo "SUCCESS"
