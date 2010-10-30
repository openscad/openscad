#!/bin/bash

cmd="./dumptest"

if [ $# == 0 ]; then
  dir=../testdata/scad
else
  dir=$1
fi

echo $dir;

for f in $dir/*.scad; do
  echo `basename $f`
  "$cmd" "$f"
done
