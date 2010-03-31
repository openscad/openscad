#!/bin/sh

VERSION=`date "+%Y.%m.%d"`
#VERSION=2010.02

export OPENCSGDIR=$PWD/../OpenCSG-1.3.0
export CGALDIR=$PWD/../install
export EIGEN2DIR=/opt/local/include/eigen2

`dirname $0`/release-common.sh -v $VERSION
if [[ $? != 0 ]]; then
  exit 1
fi
cp OpenSCAD-$VERSION.dmg ~/Documents/Dropbox/Public
ln -sf OpenSCAD-$VERSION.dmg ~/Documents/Dropbox/Public/OpenSCAD-latest.dmg

echo "Upload in progress..."
