#!/bin/sh

cmd="./cgaltest"

for f in ../testdata/scad/*-tests.scad; do
  echo == `basename $f .scad` ==
  "$cmd" "$f"
done

