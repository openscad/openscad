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
export OPENSCADDIR=$PWD
export DEPLOYDIR=$OPENSCADDIR/mingw32
export MXEDIR=$BASEDIR/mxe
export PATH=$MXEDIR/usr/bin:$PATH

echo BASEDIR: $BASEDIR
echo MXEDIR: $MXEDIR
echo DEPLOYDIR: $DEPLOYDIR
echo PATH modified with $MXEDIR/usr/bin

if [ ! -e $DEPLOYDIR ]; then
  mkdir -p $DEPLOYDIR
fi

if [ ! -h $DEPLOYDIR/mingw-cross-env ]; then
	echo linking $MXEDIR/usr/i686-pc-mingw32/ to $DEPLOYDIR/mingw-cross-env
	ln -s $MXEDIR/usr/i686-pc-mingw32/ $DEPLOYDIR/mingw-cross-env
else
	echo $DEPLOYDIR/mingw-cross-env is already symlinked
fi


