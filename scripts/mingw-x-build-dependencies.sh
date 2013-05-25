#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for cross-compilation
# from linux to mingw32/64 for windows, using the MXE cross build system.
#
# This script must be run from the OpenSCAD source root directory
#
# Usage:
#        ./scripts/mingw-x-build-dependencies.sh       # 32 bit
#        ./scripts/mingw-x-build-dependencies.sh 64    # 64 bit
#
# Prerequisites:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#
# Also note the 64 bit is built on the branch of mxe by Tony Theodore
# which hasnt been merged to official mxe as of writing

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

. ./scripts/setenv-mingw-xbuild.sh $*

if [ ! -e $BASEDIR ]; then
	mkdir -p $BASEDIR
fi

if [ ! -e $MXEDIR ]; then
	mkdir -p $MXEDIR
	cd $MXEDIR/..
	echo "Downloading MXE into " $PWD
	if [ "`echo $* | grep 64`" ]; then
		git clone -b multi-rebase git://github.com/tonytheodore/mxe.git $MXEDIR
	else
		git clone git://github.com/mxe/mxe.git $MXEDIR
	fi
fi

echo "entering" $MXEDIR
cd $MXEDIR
if [ "`echo $* | grep 64`" ]; then
  MXE_TARGETS='x86_64-w64-mingw32'
  PACKAGES='mpfr eigen opencsg cgal qt'
else
  MXE_TARGETS=
  PACKAGES='mpfr eigen opencsg cgal qt nsis'
fi
echo make $PACKAGES MXE_TARGETS=$MXE_TARGETS -j $NUMCPU JOBS=$NUMJOBS
make $PACKAGES MXE_TARGETS=$MXE_TARGETS -j $NUMCPU JOBS=$NUMJOBS

echo "leaving" $MXEDIR

echo "entering $OPENSCADDIR"
cd $OPENSCADDIR

