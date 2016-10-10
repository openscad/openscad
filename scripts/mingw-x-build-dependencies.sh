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

docmd()
{
	echo $*
	$*
}

PACKAGES='qtbase qscintilla2 mpfr eigen opencsg cgal'
PACKAGES=$PACKAGES' glib freetype fontconfig harfbuzz libxml2'
if [ "`echo $MXE_TARGET | grep i686`" ]; then
	PACKAGES=$PACKAGES' nsis'
fi

mxe_get_debian_binary()
{
	debline="deb http://pkg.mxe.cc/repos/apt/debian wheezy main"
	echo $debline > /etc/apt/sources.list.d/mxeapt.list
	apt-key adv --keyserver keyserver.ubuntu.com --recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB
	apt-get update
	MXE_PREFIX=`echo $MXE_TARGET | sed -e s/_/-/g`
	PKGLIST=
	for pkg in $PACKAGES; do
		PKGLIST=$PKGLIST' '$MXE_PREFIX'-'$pkg
	done
	apt-get -y install $PKGLIST
	apt-get install nsis
}

mxe_build_from_src()
{
	if [ ! $MXE_TARGET ]; then
		echo please run '. scripts/setenv-mingw-xbuild.sh'
		exit
	fi

	if [ ! -e $MXEDIR ]; then
		mkdir -p $MXEDIR
		cd $MXEDIR/..
		echo "Downloading MXE into " $PWD
		git clone git://github.com/openscad/mxe.git $MXEDIR
	fi

	docmd cd $MXEDIR
	docmd git checkout openscad-snapshot-build

	docmd make $PACKAGES MXE_TARGETS=$MXE_TARGET -j $NUMCPU JOBS=$NUMJOBS
	docmd cd $OPENSCADDIR
}

mxe_get_debian_binary
