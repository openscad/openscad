#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for cross-compilation
# from linux to mingw32/64 for windows, using the MXE cross build system.
#
# This script must be run from the OpenSCAD source root directory
#
# Usage:
#        ./scripts/mingw-x-build-dependencies.sh              # 32 bit
#        ./scripts/mingw-x-build-dependencies.sh 64           # 64 bit
#
# If you just want to download, and build later:
#
#        ./scripts/mingw-x-build-dependencies.sh download     # 32 bit download
#        ./scripts/mingw-x-build-dependencies.sh 64 download  # 64 bit download
#
# Prerequisites:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#
# Notes:
#
# Originally this was based on Tony Theodore's branch of MXE, which is now
# integrated into official MXE.
#
# Targets:
#
# MXE allows 4 separate targets with the MXE_TARGETS environment variable.
# As of 2015 shared are not guaranteed to work.
#
# 64 bit static linked libraries MXE_TARGETS=x86_64-w64-mingw32.static
# 32 bit static linked libraries MXE_TARGETS=i686-w64-mingw32.static
# 64 bit shared libraries        MXE_TARGETS=x86_64-w64-mingw32.shared
# 32 bit shared libraries        MXE_TARGETS=i686-w64-mingw32.shared
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

. ./scripts/setenv-mingw-xbuild.sh $*

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
echo 'checkout openscad-snapshot-build branch'
git checkout openscad-snapshot-build
if [ "`echo $* | grep 64`" ]; then
 MXE_TARGETS='x86_64-w64-mingw32.static'
 if [ "`echo $* | grep download`" ]; then
  PACKAGES='download-mpfr download-eigen download-opencsg download-cgal download-qtbase download-glib download-libxml2 download-freetype download-fontconfig download-harfbuzz'
 else
  PACKAGES='qtbase qscintilla2 mpfr eigen opencsg cgal glib libxml2 freetype fontconfig harfbuzz'
 fi
else
 MXE_TARGETS='i686-w64-mingw32.static'
 if [ "`echo $* | grep download`" ]; then
  PACKAGES='download-mpfr download-eigen download-opencsg download-cgal download-qtbase download-nsis download-glib download-libxml2 download-freetype download-fontconfig download-harfbuzz'
 else
  PACKAGES='qtbase qscintilla2 mpfr eigen opencsg cgal nsis glib libxml2 freetype fontconfig harfbuzz'
 fi
fi
echo make $PACKAGES MXE_TARGETS=$MXE_TARGETS -j $NUMCPU JOBS=$NUMJOBS
make $PACKAGES MXE_TARGETS=$MXE_TARGETS -j $NUMCPU JOBS=$NUMJOBS

echo "leaving" $MXEDIR

echo "entering $OPENSCADDIR"
cd $OPENSCADDIR

