#!/bin/sh

# Set this if we're doing a release build. Comment it out for development builds
#VERSION=2011.12

if test -z "$VERSION"; then
  VERSION=`date "+%Y.%m.%d"`
  COMMIT=-c
fi

# Turn off ccache, just for safety
CCACHE_DISABLE=1

. ./scripts/setenv-mingw-xbuild.sh

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

OS=LINXWIN ./scripts/release-common.sh -v $VERSION $COMMIT
if [[ $? != 0 ]]; then
  exit 1
fi

echo "Please upload these files to the appropriate location"
BINFILE=$DEPLOYDIR/OpenSCAD-$VERSION.zip
INSTALLFILE=$DEPLOYDIR/OpenSCAD-$VERSION-Installer.exe
echo $BINFILE
echo $INSTALLFILE

echo
echo "Then copy/paste these commands into your shell:"
echo

# Update snapshot filename on wab page
echo ./scripts/update-web.sh OpenSCAD-$VERSION.zip
echo ./scripts/update-web.sh OpenSCAD-$VERSION-Installer.exe
