#!/bin/sh

VERSION=`date "+%Y.%m.%d"`
#VERSION=2011.06

# Turn off ccache, just for safety
PATH=${PATH//\/opt\/local\/libexec\/ccache:}

# This is the same location as DEPLOYDIR in macosx-build-dependencies.sh
export OPENSCAD_LIBRARIES=$PWD/../libraries/install

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

# Update snapshot filename on wab page
`dirname $0`/update-web.sh OpenSCAD-$VERSION.dmg
