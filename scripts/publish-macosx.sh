#!/bin/sh

VERSION=`date "+%Y.%m.%d"`
#VERSION=2010.02

export MACOSX_DEPLOY_DIR=$PWD/../libraries/deploy

`dirname $0`/release-common.sh -v $VERSION
if [[ $? != 0 ]]; then
  exit 1
fi
`dirname $0`/macosx-sanity-check.py OpenSCAD.app/Contents/MacOS/OpenSCAD
if [[ $? != 0 ]]; then
  exit 1
fi
cp OpenSCAD-$VERSION.dmg ~/Documents/Dropbox/Public
ln -sf OpenSCAD-$VERSION.dmg ~/Documents/Dropbox/Public/OpenSCAD-latest.dmg

echo "Upload in progress..."
