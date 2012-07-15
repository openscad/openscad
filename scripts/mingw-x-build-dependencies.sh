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
	echo "note: you can 'export NUMCPU=x' for paralell builds (x=number)";
	NUMCPU=1
fi
echo NUMCPU: $NUMCPU

echo "Downloading MXE into " $MXEDIR
cd $BASEDIR

git clone git://github.com/mxe/mxe.git

cd $MXEDIR
make -j $NUMCPU JOBS=$NUMCPU mpfr eigen opencsg cgal qt
#make -j $NUMCPU JOBS=1 mpfr # for testing
echo 'make'

echo "leaving" $MXEDIR
echo "entering $OPENSCADDIR"
cd $OPENSCADDIR
if [ ! -e mingw-cross-env ]; then
	ln -s $MXEDIR mingw-cross-env
fi

echo
echo "now copy/paste the following to cross-build openscad"
echo
echo "i686-pc-mingw32-qmake CONFIG+=mingw-cross-env openscad.pro"
echo "make"
echo
