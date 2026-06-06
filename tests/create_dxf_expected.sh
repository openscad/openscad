#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <version>  (e.g. R14, R12, R10, legacy)"
    exit 1
fi

VERSION=$1
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mkdir -p "${SCRIPT_DIR}/regression/render-dxf/${VERSION}"

for f in "${SCRIPT_DIR}/data/scad/dxf/"*.scad \
          "${SCRIPT_DIR}/data/scad/2D/features/"*.scad \
          "${SCRIPT_DIR}/data/scad/2D/issues/"*.scad \
          "${SCRIPT_DIR}/data/scad/misc/"*.scad; do
    [ -f "$f" ] || continue
    base=$(basename "$f" .scad)
    printf "DXF audit passed.\n" \
        > "${SCRIPT_DIR}/regression/render-dxf/${VERSION}/${base}-expected.txt"
done

printf "DXF audit passed.\n" \
    > "${SCRIPT_DIR}/regression/render-dxf/${VERSION}/example015-expected.txt"
printf "DXF audit passed.\n" \
    > "${SCRIPT_DIR}/regression/render-dxf/${VERSION}/module_recursion-expected.txt"
printf "DXF audit passed.\n" \
    > "${SCRIPT_DIR}/regression/render-dxf/${VERSION}/list_comprehensions-expected.txt"
printf "DXF audit passed.\n" \
    > "${SCRIPT_DIR}/regression/render-dxf/${VERSION}/polygon_areas-expected.txt"
printf "DXF audit passed.\n" \
    > "${SCRIPT_DIR}/regression/render-dxf/${VERSION}/recursion-expected.txt"

echo "Created expected files in ${SCRIPT_DIR}/regression/render-dxf/${VERSION}"
