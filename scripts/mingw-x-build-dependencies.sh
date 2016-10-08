#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for cross-compilation
# from linux to mingw32/64 for windows, using the MXE cross build system.
#
# This script must be run from the OpenSCAD source root directory
#
# Usage:
#        ./scripts/mingw-x-build-dependencies.sh [64|32] [download]
#
# 64 or 32 choose the bits of the target operating system. Default is 64
# download will perform only the download, not the build.
#
# Prerequisites:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#
# Targets:
#
# MXE allows separate targets with the MXE_TARGETS environment variable.
#
# As of 2016:
# 64 bit static linked libraries MXE_TARGETS=x86_64-w64-mingw32.static
# 32 bit static linked libraries MXE_TARGETS=i686-w64-mingw32.static
# 64 bit shared libraries        MXE_TARGETS=x86_64-w64-mingw32.shared
# 32 bit shared libraries        MXE_TARGETS=i686-w64-mingw32.shared
#

if [ ! $OPENSCADDIR ]; then
	echo please run '. scripts/setenv-mingw-xbuild.sh'
fi
if [ ! $MXE_TARGET ]; then
	echo please run '. scripts/setenv-mingw-xbuild.sh'
fi

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

if [ ! -e $MXEDIR ]; then
	mkdir -p $MXEDIR
	cd $MXEDIR/..
	echo "Downloading MXE into " $PWD
	git clone git://github.com/openscad/mxe.git $MXEDIR
fi

echo "entering" $MXEDIR
cd $MXEDIR
echo 'checkout openscad-snapshot-build branch'
git checkout openscad-snapshot-build

PACKAGES='qtbase qscintilla2 mpfr eigen opencsg cgal'
PACKAGES=$PACKAGES' glib freetype fontconfig harfbuzz'
if [ "`echo $MXE_TARGET|grep i686`" ]; then
  PACKAGES=$PACKAGES' nsis'
fi

echo make $PACKAGES MXE_TARGETS=$MXE_TARGET -j $NUMCPU JOBS=$NUMJOBS
make $PACKAGES MXE_TARGETS=$MXE_TARGET -j $NUMCPU JOBS=$NUMJOBS

echo "leaving" $MXEDIR

echo "entering $OPENSCADDIR"
cd $OPENSCADDIR

