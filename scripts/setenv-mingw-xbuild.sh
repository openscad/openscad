#!/bin/sh -e
#
# set environment variables for mingw/mxe cross-build
#
# Usage:
#
#  source ./scripts/setenv-mingw-xbuild.sh [32|64] [shared|static] [qt4]
#
# Prerequisites:
#
#  Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#

OPENSCADDIR=$PWD
if [ ! -e openscad.pro ]; then
	echo "please run from root openscad source"
fi

if [ ! $BASEDIR ]; then
	BASEDIR=$HOME/openscad_deps
fi

if [ ! $MXEDIR ]; then
	MXEDIR=$BASEDIR/mxe
	if [ ! -e $MXEDIR ]; then
		if [ -e /opt/mxe ]; then
			MXEDIR=/opt/mxe # mxe on custom build machines
		fi
		#if [ -e /usr/lib/mxe ]; then
		#	MXEDIR=/usr/lib/mxe  # mxe dpkg binary on debian
		#fi
	fi
fi

MXE_TRIPLET_64=x86_64-w64-mingw32
MXE_TRIPLET_32=i686-w64-mingw32

MXE_TRIPLET=$MXE_TRIPLET_64
if [ "`echo $* | grep 32 `" ]; then
	MXE_TRIPLET=$MXE_TRIPLET_32
fi

MXE_LIB_TYPE=static
if [ "`echo $* | grep shared `" ]; then
	MXE_LIB_TYPE=shared
fi

MXE_QTSUBDIR=qt5
if [ "`echo $* | grep qt4 `" ]; then
	MXE_QTSUBDIR=qt
fi

MXE_TARGET=$MXE_TRIPLET'.'$MXE_LIB_TYPE
MXE_TARGET_DIR=$MXEDIR/usr/$MXE_TARGET
MXE_QTBIN_DIR=$MXE_TARGET_DIR/$MXE_QTSUBDIR/bin
MXE_BIN_DIR=$MXEDIR/usr/bin
DEPLOYDIR=$OPENSCADDIR/bin/$MXE_TARGET
OPENSCAD_LIBRARIES=$MXE_TARGET_DIR

cleanpath()
{
	D1=$MXEDIR/usr/$MXE_TRIPLET_32.s...../qt/bin:
	D2=$MXEDIR/usr/$MXE_TRIPLET_32.s...../qt5/bin:
	D3=$MXEDIR/usr/$MXE_TRIPLET_64.s...../qt/bin:
	D4=$MXEDIR/usr/$MXE_TRIPLET_64.s...../qt5/bin:
	PATH=`echo $PATH | sed -e s~$D1~~g`
	PATH=`echo $PATH | sed -e s~$D2~~g`
	PATH=`echo $PATH | sed -e s~$D3~~g`
	PATH=`echo $PATH | sed -e s~$D4~~g`
	PATH=`echo $PATH | sed -e s~$MXEDIR/usr/bin:~~g`
}

cleanpath
PATH=$MXE_BIN_DIR:$MXE_QTBIN_DIR:$PATH

export OPENSCADDIR
export BASEDIR
export MXEDIR
export MXE_TARGET
export MXE_LIB_TYPE
export MXE_TARGET_DIR
export DEPLOYDIR
export OPENSCAD_LIBRARIES
export PATH

echo OPENSCADDIR: $OPENSCADDIR
echo BASEDIR: $BASEDIR
echo MXEDIR: $MXEDIR
echo MXE_TARGET: $MXE_TARGET
echo MXE_LIB_TYPE: $MXE_LIB_TYPE
echo MXE_TARGET_DIR: $MXE_TARGET_DIR
echo DEPLOYDIR: $DEPLOYDIR
echo OPENSCAD_LIBRARIES: $OPENSCAD_LIBRARIES
echo PATH: $PATH

if [ ! -e $DEPLOYDIR ]; then
	echo creating $DEPLOYDIR
	mkdir -p $DEPLOYDIR
fi

