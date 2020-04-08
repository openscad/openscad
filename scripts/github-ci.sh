#!/bin/bash

set -e

PARALLEL=-j2
PARALLEL_CTEST=-j2

BUILDDIR=b
TESTDIR=tests

rm -rf "$BUILDDIR"
mkdir "$BUILDDIR"
(
	cd "$BUILDDIR" && cmake -DCMAKE_BUILD_TYPE=Release -DEXPERIMENTAL=ON .. && make $PARALLEL
)

ln -s "$BUILDDIR"/openscad
cd "$TESTDIR" && cmake . && make

ctest $PARALLEL_CTEST
if [[ $? != 0 ]]; then
  echo "Test failure"
  exit 1
fi
