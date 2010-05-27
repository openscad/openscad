#!/bin/sh

SCRIPTDIR=`dirname $0`

mkdir -p output
for f in examples/*.scad; do
  echo `basename $f .scad`
  $SCRIPTDIR/create-stl.sh $f output/`basename $f .scad`.stl
done
