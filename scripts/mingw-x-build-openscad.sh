#!/bin/sh -e
#
# This script builds Openscad.exe for windows using the MXE cross build system.
#
# This script must be run from the OpenSCAD source root directory
#
# You must have already run the mingw-x-build-dependencies.sh script
#
# Usage:
#
# ./scripts/mingw-x-build-openscad.sh
#

OPENSCADDIR=$PWD
if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi
echo OPENSCADDIR: $OPENSCADDIR

. ./scripts/setenv-mingw-xbuild.sh

if [ ! -e $BASEDIR/lib ]; then
	echo "please run the mingw-x-build-dependencies.sh script first"
fi

echo "entering $DEPLOYDIR"
cd $DEPLOYDIR
i686-pc-mingw32-qmake CONFIG+=mingw-cross-env ../openscad.pro
#"make -j$NUMCPU" # causes parser_yacc.hpp errors
make
echo "leaving $DEPLOYDIR"

echo "entering $OPENSCADDIR"
cd $OPENSCADDIR

OPENSCAD_EXE=$DEPLOYDIR/release/openscad.exe

if [ -e $OPENSCAD_EXE ] ; then
	echo Build finished. Executable:
	echo
	echo " "$OPENSCAD_EXE
	echo
else
	echo Cannot find $OPENSCAD_EXE. The build appears to have had an error.
fi

