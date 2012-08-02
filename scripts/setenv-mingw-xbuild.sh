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

OPENSCADDIR=$PWD

if [ ! $BASEDIR ]; then
	BASEDIR=$HOME/openscad_deps
fi
DEPLOYDIR=$OPENSCADDIR/mingw32
MXEDIR=$BASEDIR/mxe
PATH=$MXEDIR/usr/bin:$PATH

echo MXEDIR: $MXEDIR
echo BASEDIR: $BASEDIR
echo DEPLOYDIR: $DEPLOYDIR
echo OPENSCADDIR: $OPENSCADDIR
echo PATH modified with $MXEDIR/usr/bin
if [ ! $NUMCPU ]; then
	echo "note: you can 'export NUMCPU=x' for multi-core compiles (x=number)";
	NUMCPU=1
fi
if [ ! $NUMJOBS ]; then
	echo "note: you can 'export NUMJOBS=x' for building multiple pkgs at once"
	if [ $NUMCPU -gt 2 ]; then
		NUMJOBS=$((NUMCPU/2))
	else
		NUMJOBS=1
	fi
fi
echo NUMCPU: $NUMCPU
echo NUMJOBS: $NUMJOBS

