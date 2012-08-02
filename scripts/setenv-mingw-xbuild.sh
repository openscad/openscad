#!/bin/sh -e
#
# set environment variables for mingw/mxe cross-build
#
# Usage: source ./scripts/setenv-mingw-xbuild.sh
#
# Prerequisites:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#

if [ ! $BASEDIR ]; then
	BASEDIR=$HOME/openscad_deps
fi
export DEPLOYDIR=$OPENSCADDIR/mingw32
export MXEDIR=$BASEDIR/mxe
export PATH=$MXEDIR/usr/bin:$PATH

echo MXEDIR: $MXEDIR
echo BASEDIR: $BASEDIR
echo DEPLOYDIR: $DEPLOYDIR
echo PATH modified with $MXEDIR/usr/bin

