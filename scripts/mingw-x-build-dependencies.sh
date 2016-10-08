#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for cross-compilation
# from linux to mingw32/64 for windows(TM), using the MXE cross build system.
#
# Usage:
#   . ./scripts/setenv-mingw-xbuild.sh [64|32] [static|shared]
#   ./scripts/mingw-x-build-dependencies.sh
#
# As of 2016 MXE allows these targets with the MXE_TARGETS environment variable.
#
# 64 bit static linked libraries MXE_TARGETS=x86_64-w64-mingw32.static
# 32 bit static linked libraries MXE_TARGETS=i686-w64-mingw32.static
# 64 bit shared libraries        MXE_TARGETS=x86_64-w64-mingw32.shared
# 32 bit shared libraries        MXE_TARGETS=i686-w64-mingw32.shared
#
# Prerequisites:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#
#

if [ ! $MXE_TARGET ]; then
	echo please run '. scripts/setenv-mingw-xbuild.sh'
	exit
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

docmd()
{
	echo $*
	$*
}

docmd cd $MXEDIR
docmd git checkout openscad-snapshot-build

PACKAGES='qtbase qscintilla2 mpfr eigen opencsg cgal'
PACKAGES=$PACKAGES' glib freetype fontconfig harfbuzz'
if [ "`echo $MXE_TARGET|grep i686`" ]; then
  PACKAGES=$PACKAGES' nsis'
fi

docmd echo make $PACKAGES MXE_TARGETS=$MXE_TARGET -j $NUMCPU JOBS=$NUMJOBS
docmd cd $OPENSCADDIR
