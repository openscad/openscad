#!/bin/sh -e
#
# set environment variables for mingw/mxe cross-build
#
# Usage:
#
#     source ./scripts/setenv-mingw-xbuild.sh         # 32 bit build
#     source ./scripts/setenv-mingw-xbuild.sh 64      # 64 bit build
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
	if [ `echo $* | grep 64 ` ]; then
		DEPLOYDIR=$OPENSCADDIR/mingw64
	else
		DEPLOYDIR=$OPENSCADDIR/mingw32
	fi
	export DEPLOYDIR
fi

if [ ! $MXEDIR ]; then
	if [ `echo $* | grep 64 ` ]; then
		export MXEDIR=$BASEDIR/mxe-w64
	else
		export MXEDIR=$BASEDIR/mxe
	fi
fi

export PATH=$MXEDIR/usr/bin:$PATH

if [ ! -e $DEPLOYDIR ]; then
  mkdir -p $DEPLOYDIR
fi

if [ `echo $* | grep 64 ` ]; then
	MXETARGETDIR=$MXEDIR/usr/x86_64-w64-mingw32
else
	MXETARGETDIR=$MXEDIR/usr/i686-pc-mingw32
fi
echo linking $MXETARGETDIR to $DEPLOYDIR/mingw-cross-env
rm -f $DEPLOYDIR/mingw-cross-env
ln -s $MXETARGETDIR $DEPLOYDIR/mingw-cross-env
export PATH=$MXETARGETDIR/qt/bin:$PATH

echo BASEDIR: $BASEDIR
echo MXEDIR: $MXEDIR
echo DEPLOYDIR: $DEPLOYDIR
echo PATH modified: $MXEDIR/usr/bin
echo PATH modified: $MXETARGETDIR/qt/bin



