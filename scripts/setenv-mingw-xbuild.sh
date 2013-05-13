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

export OPENSCADDIR=$PWD

if [ ! $BASEDIR ]; then
	export BASEDIR=$HOME/openscad_deps
fi

if [ ! $DEPLOYDIR ]; then
	export DEPLOYDIR=$OPENSCADDIR/mingw32
fi

if [ ! $MXEDIR ]; then
	export MXEDIR=$BASEDIR/mxe
fi

export PATH=$MXEDIR/usr/bin:$PATH

if [ ! -e $DEPLOYDIR ]; then
  mkdir -p $DEPLOYDIR
fi

echo linking $MXEDIR/usr/i686-pc-mingw32/ to $DEPLOYDIR/mingw-cross-env
rm -f $DEPLOYDIR/mingw-cross-env
ln -s $MXEDIR/usr/i686-pc-mingw32/ $DEPLOYDIR/mingw-cross-env
export PATH=$MXEDIR/usr/i686-pc-mingw32/qt/bin:$PATH

echo BASEDIR: $BASEDIR
echo MXEDIR: $MXEDIR
echo DEPLOYDIR: $DEPLOYDIR
echo PATH modified: $MXEDIR/usr/bin
echo PATH modified: $MXEDIR/usr/i686-pc-mingw32/qt/bin



