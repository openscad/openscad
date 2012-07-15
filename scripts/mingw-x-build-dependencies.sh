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
BASEDIR=$HOME/openscad_deps
MXEDIR=$BASEDIR/mxe
PATH=$MXEDIR/usr/bin:$PATH

echo MXEDIR: $MXEDIR
echo BASEDIR: $BASEDIR
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

cd $BASEDIR
if [ ! -e mxe ]; then
	echo "Downloading MXE into " $MXEDIR
	git clone git://github.com/mxe/mxe.git
fi

echo "entering" $MXEDIR
cd $MXEDIR
echo "make mpfr eigen opencsg cgal qt -j $NUMCPU JOBS=$NUMJOBS"
make mpfr eigen opencsg cgal qt -j $NUMCPU JOBS=$NUMJOBS
#make mpfr -j$NUMCPU JOBS=$NUMJOBS # for testing
echo 'make'

echo "leaving" $MXEDIR
echo "entering $OPENSCADDIR"
cd $OPENSCADDIR
if [ -e mingw-cross-env ]; then
	rm ./mingw-cross-env
fi
echo "linking mingw-cross-env directory"
ln -s $MXEDIR/usr/i686-pc-mingw32/ ./mingw-cross-env

echo
echo "now copy/paste the following to cross-build openscad"
echo
echo "export PATH=$MXEDIR/usr/bin:\$PATH"
echo "i686-pc-mingw32-qmake CONFIG+=mingw-cross-env openscad.pro"
#echo "make -j$NUMCPU" # causes parser_yacc.hpp errors
echo "make"
echo
