#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for cross-compilation
# from linux to mingw32/64 for windows, using the MXE cross build system.
#
# This script must be run from the OpenSCAD source root directory
#
# This script must be run after running the following:
#
#  source ./scripts/setenv-mingw-xbuild.sh
#
# It will create the build target (32/64 static/shared) as given to that script.
#
# Usage:
#
# ./scripts/mingw-x-build-dependencies.sh download # download only
# ./scripts/mingw-x-build-dependencies.sh          # build targets from setenv
#
# Prerequisites:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#
# Shout out to Tony Theodore for updating MXE to 64 bit
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

if [ ! $OPENSCAD_BUILD_TARGET_OSTYPE ]; then
  echo "please run 'source ./scripts/setenv-mingw-xbuild.sh' first"
  echo "please check the README.MD"
fi

if [ ! -e $BASEDIR ]; then
	mkdir -p $BASEDIR
fi

if [ ! -e $MXEDIR ]; then
	mkdir -p $MXEDIR
	cd $MXEDIR/..
	echo "Downloading MXE into " $PWD
	git clone git://github.com/openscad/mxe.git $MXEDIR
fi

echo "entering" $MXEDIR
cd $MXEDIR
plist=""
plist="$plist qtbase qscintilla2 mpfr eigen opencsg cgal glib freetype"
plist="$plist fontconfig harfbuzz nsis"
if [ "`echo $* | grep download`" ]; then
  plist2=
  for pkg in $plist; do
    plist2="$plist2 "$pkg"-download"
  done
  plist=$plist2
fi
cmd="make MXE_TARGETS=$MXE_TARGET -j $NUMCPU JOBS=$NUMJOBS $plist"
echo $cmd
eval $cmd

echo "leaving" $MXEDIR
echo "entering $OPENSCADDIR"
cd $OPENSCADDIR

