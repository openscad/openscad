#!/bin/sh -e
#
# This script builds a binary install package of OpenSCAD for Windows
# using a cross-built mingw OpenSCAD and the NSIS installer system
#
# This script must be run from the OpenSCAD source root directory
#
# Usage: ./scripts/mingw-x-build-installer.sh
#
# Result: binary installer in $DEPLOYDIR directory
#
#
# Prerequisites:
#
#   source ./scripts/setenv-mingw-xbuild.sh
#   ./scripts/mingw-x-build-dependencues.sh
#
# and then build openscad before running this script.
#
# You need MCAD. run 'git submodule init && git submodule update'
#
# You need the Nullsoft installer system, on ubuntu 'sudo apt-get install nsis'
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#

OPENSCADDIR=$PWD
if [ ! -f $OPENSCADDIR/openscad.pro ]; then
	echo "Must be run from the OpenSCAD source root directory"
	exit 1
fi

. ./scripts/setenv-mingw-xbuild.sh

if [ ! -e $OPENSCADDIR/libraries/MCAD/__init__.py ]; then
	echo "Downloading MCAD"
	git submodule init
	git submodule update
fi

if [ ! -e $DEPLOYDIR ]; then
	echo $DEPLOYDIR "empty. Please build OpenSCAD for mingw32 first."
	exit 1
fi

OPENSCAD_EXE=$DEPLOYDIR/release/openscad.exe

if [ ! -e $OPESCAD_EXE ]; then
	echo "Can't find" $OPENSCAD_EXE "Please build OpenSCAD for mingw32 first."
	exit 1
fi

MAKENSIS=

if [ "`command -v makensis`" ]; then
	MAKENSIS=makensis
fi

if [ "`command -v i686-pc-mingw32-makensis`" ]; then
	MAKENSIS=i686-pc-mingw32-makensis
fi

if [ ! $MAKENSIS ]; then
	echo "makensis not found. please install nsis"
	exit 1
fi

echo "Copying files to" $DEPLOYDIR

copy_files()
{
	echo "copying" $1
	cp -af $1 $2
}

copy_files $OPENSCADDIR/libraries $DEPLOYDIR
copy_files $OPENSCADDIR/examples $DEPLOYDIR
copy_files $OPENSCADDIR/scripts/installer.nsi $DEPLOYDIR
copy_files $OPENSCADDIR/scripts/mingw-file-association.nsh $DEPLOYDIR

echo running $MAKENSIS in $DEPLOYDIR

NSIS_DEBUG_FLAG=-V2
#NSIS_DEBUG_FLAG=    # leave blank for full log
cd $DEPLOYDIR && $MAKENSIS $NSIS_DEBUG_FLAG installer.nsi

cd $OPENSCADDIR

INSTALLFILE=$DEPLOYDIR/openscad_setup.exe

if [ -e $INSTALLFILE ]; then
	echo "Build complete. Install file ready:"
	echo " " $INSTALLFILE
else
	echo "Build failed. Sorry."
fi

