#!/bin/sh

VERSION=`date "+%Y.%m.%d"`
#VERSION=2010.02

export OPENCSGDIR=$PWD/../OpenCSG-1.2.0

`dirname $0`/release-common.sh -v $VERSION
cp OpenSCAD-$VERSION.dmg ~/Documents/Dropbox/Public
ln -sf OpenSCAD-$VERSION.dmg ~/Documents/Dropbox/Public/OpenSCAD-latest.dmg

echo "Upload in progress..."
