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
# You need to copy/paste the FileAssociation.nsh file from
# http://nsis.sourceforge.net/File_Association into DEPLOYDIR
# (it has no license information so cannot be included directly)
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

if [ ! -e $DEPLOYDIR/openscad.exe ]; then
	echo "Can't find" $DEPLOYDIR"/openscad.exe Please build OpenSCAD for mingw32 first."
	exit 0
fi

if [ ! "`command -v makensis`" ]; then
	echo "makensis not found. please install nsis"
	exit 0
fi

if [ ! -e $DEPLOYDIR/FileAssociation.nsh ]; then
	echo "Please install FileAssociation.nsh into" $DEPLOYDIR
	echo "You can copy/paste it from http://nsis.sourceforge.net/File_Association"
fi

echo "Copying files to" $DEPLOYDIR

cp -av $OPENSCADDIR/libraries $DEPLOYDIR
cp -av $OPENSCADDIR/examples $DEPLOYDIR
cp -av $OPENSCADDIR/scripts/installer.nsi $DEPLOYDIR

echo "running nsis"

cd $DEPLOYDIR && makensis installer.nsi

cd $OPENSCADDIR

INSTALLFILE=$DEPLOYDIR/openscad_setup.exe

if [ -e $INSTALLFILE ]; then
	echo "Build complete. Install file ready: $INSTALLFILE"
else
	echo "Build failed. Sorry."
fi

