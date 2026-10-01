#!/usr/bin/env bash
#
# Does a light check of a node or web WASM build.
#
set -euo pipefail

ENV=$1

rm -f out.stl
case $ENV in
  node)
    echo "Checking WASM node build..."
    ../build/openscad.js \
        --backend=manifold \
        examples/Basics/CSG.scad \
        -o out.stl
    cat out.stl
    ;;
  node-module)
    echo "Checking WASM node module build..."
    node --input-type=module -e "
      import OpenSCAD from '$PWD/../build/openscad.js';
      const instance = await OpenSCAD({noInitialRun: true});
      instance.callMain(['--backend=manifold', 'examples/Basics/CSG.scad', '-o', 'out.stl']);
    "
    cat out.stl
    ;;
  web)
    echo "Checking WASM web build..."
    file="tests/wasm-check.html"
    node tests/wasm-check.mjs $PWD/$file
    ;;
  *)
    echo "Usage: $0 {node|node-module|web}"
    exit 1
    ;;
esac 2>&1 | tee out.log
cat out.log | grep "Top level object is a 3D object (manifold)" >/dev/null
echo "SUCCESS"
