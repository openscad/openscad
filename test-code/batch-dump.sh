#!/bin/sh

cmd="./dumptest"

if [ $# == 0 ]; then
  dir=../testdata/scad
else
  dir=$1
fi

for f in $dir/*.scad; do
  echo `basename $f`
  "$cmd" "$f"
done
