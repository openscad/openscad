#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for cross-compilation
# from linux to mingw32 for windows, using the MXE cross build system.
#
# This script must be run from the OpenSCAD source root directory
#
# Usage: ./scripts/mingw-x-build-dependencies.sh
#
# Prerequisites:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#

OPENSCADDIR=$PWD
if [ ! -f $OPENSCADDIR/openscad.pro ]; then
	echo "Must be run from the OpenSCAD source root directory"
	exit 0
fi
echo OPENSCADDIR: $OPENSCADDIR

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

. ./scripts/setenv-mingw-xbuild.sh

if [ ! -e $BASEDIR ]; then
	mkdir -p $BASEDIR
fi

if [ ! -e $MXEDIR ]; then
	mkdir -p $MXEDIR
	cd $MXEDIR/..
	echo "Downloading MXE into " $PWD
	git clone git://github.com/mxe/mxe.git
fi

echo "entering" $MXEDIR
cd $MXEDIR
echo "make mpfr eigen opencsg cgal qt nsis -j $NUMCPU JOBS=$NUMJOBS"
make mpfr eigen opencsg cgal qt nsis -j $NUMCPU JOBS=$NUMJOBS
#make mpfr -j $NUMCPU JOBS=$NUMJOBS # for testing

echo "leaving" $MXEDIR

echo "entering $OPENSCADDIR"
cd $OPENSCADDIR

