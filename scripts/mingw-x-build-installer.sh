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
	exit 0
fi

. ./scripts/setenv-mingw-xbuild.sh

if [ ! -e $OPENSCADDIR/libraries/MCAD ]; then
	echo "Downloading MCAD"
	git submodule init
	git submodule update
fi

if [ ! -e $DEPLOYDIR ]; then
	echo $DEPLOYDIR "empty. Please build OpenSCAD for mingw32 first."
	exit 0
fi

OPENSCAD_EXE=$DEPLOYDIR/release/openscad.exe

if [ ! -e $OPESCAD_EXE ]; then
	echo "Can't find" $OPENSCAD_EXE "Please build OpenSCAD for mingw32 first."
	exit 0
fi

if [ ! "`command -v makensis`" ]; then
	echo "makensis not found. please install nsis"
	exit 0
fi


echo "Copying files to" $DEPLOYDIR

copy_files()
{
	echo "copying" $1
	cp -a $1 $2
}

copy_files $OPENSCADDIR/libraries $DEPLOYDIR
copy_files $OPENSCADDIR/examples $DEPLOYDIR
copy_files $OPENSCADDIR/scripts/installer.nsi $DEPLOYDIR
copy_files $OPENSCADDIR/scripts/mingw-file-association.nsh $DEPLOYDIR

echo "running makensis in" $DEPLOYDIR

cd $DEPLOYDIR && makensis -V2 installer.nsi
# cd $DEPLOYDIR && makensis installer.nsi # debug nsis

cd $OPENSCADDIR

INSTALLFILE=$DEPLOYDIR/openscad_setup.exe

if [ -e $INSTALLFILE ]; then
	echo "Build complete. Install file ready:"
	echo " " $INSTALLFILE
else
	echo "Build failed. Sorry."
fi

