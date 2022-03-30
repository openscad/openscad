#!/bin/sh

# This is run as part of the checklist in docs/release-checklist.txt
#

if test -z "$VERSION"; then
  VERSION=`date "+%Y.%m.%d"`
  COMMIT=-c
fi

# Turn off ccache, just for safety
CCACHE_DISABLE=1

. ./scripts/setenv-mingw-xbuild.sh

if [ ! -e $MXEDIR ]; then
  echo "MXEDIR: $MXEDIR"
  echo "MXEDIR is a non-existent path. Mingw cross tools not found."
  echo
  echo " Please run ./scripts/mingw-x-build-dependencies.sh to install MXE"
  echo " or modify MXEDIR to point to the root of your cross-tools setup"
  echo " ( Please see http://mxe.cc for more info ) "
  echo
  exit 1
fi

if [ ! -f $OPENSCADDIR/openscad.appdata.xml.in ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 1
fi

./scripts/release-common.sh -v $VERSION $COMMIT mingw32

if [ $? != 0 ]; then
	echo "release-common.sh returned error code: $?. build stopped."
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

# Update snapshot filename on web page
echo ./scripts/update-web.sh OpenSCAD-$VERSION.zip
echo ./scripts/update-web.sh OpenSCAD-$VERSION-Installer.exe
