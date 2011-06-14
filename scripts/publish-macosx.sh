#!/bin/sh

#VERSION=`date "+%Y.%m.%d"`
VERSION=2011.06

# This is the same location as DEPLOYDIR in macosx-build-dependencies.sh
export MACOSX_DEPLOY_DIR=$PWD/../libraries/install

`dirname $0`/release-common.sh -v $VERSION
if [[ $? != 0 ]]; then
  exit 1
fi

echo "Sanity check of the app bundle..."
`dirname $0`/macosx-sanity-check.py OpenSCAD.app/Contents/MacOS/OpenSCAD
if [[ $? != 0 ]]; then
  exit 1
fi
cp OpenSCAD-$VERSION.dmg ~/Dropbox/Public
ln -sf OpenSCAD-$VERSION.dmg ~/Dropbox/Public/OpenSCAD-latest.dmg

echo "Upload in progress..."
