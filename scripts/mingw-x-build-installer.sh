#!/bin/sh -e
#
# This script builds a binary install package of OpenSCAD for Windows
# using a cross-built mingw OpenSCAD and the NSIS installer system
#
# This script must be run from the OpenSCAD source root directory
#
# Usage: ./scripts/mingw-x-build-release.sh
#
# Result: binary installer in ./release_mingw32 directory
#
#
# Prerequisites:
#
# You need to run the scripts/mingw-x-build-dependencues.sh script
# and build openscad before running this script.
#
# You need MCAD. run 'git submodule init && git submodule update'
#
# You need the Nullsoft installer system, on ubuntu 'sudo apt-get install nsis'
#
# You need to copy/paste the FileAssociation.nsh file from
# http://nsis.sourceforge.net/File_Association into RELEASE_DIR
# (it has no license information so cannot be included directly)
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#

OPENSCADDIR=$PWD
RELEASE_DIR=$OPENSCADDIR/release_mingw32

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
	echo "Must be run from the OpenSCAD source root directory"
	exit 0
fi

if [ ! -e $OPENSCADDIR/libraries/MCAD ]; then
	echo "Please download the MCAD submodule: (git submodule init && git submodule update)"
	exit 0
fi

if [ ! -e $RELEASE_DIR ]; then
	echo $RELEASE_DIR "empty. Please build OpenSCAD for mingw32 first."
	exit 0
fi

if [ ! -e $RELEASE_DIR/openscad.exe ]; then
	echo "Can't find" $RELEASE_DIR"/openscad.exe Please build OpenSCAD for mingw32 first."
	exit 0
fi

if [ ! "`command -v makensis`" ]; then
	echo "makensis not found. please install nsis"
	exit 0
fi

if [ ! -e $RELEASE_DIR/FileAssociation.nsh ]; then
	echo "Please install FileAssociation.nsh into" $RELEASE_DIR
	echo "You can copy/paste it from http://nsis.sourceforge.net/File_Association"
fi

echo "Copying files to" $RELEASE_DIR

cp -av $OPENSCADDIR/libraries $RELEASE_DIR
cp -av $OPENSCADDIR/examples $RELEASE_DIR
cp -av $OPENSCADDIR/scripts/installer.nsi $RELEASE_DIR

echo "running nsis"

cd $RELEASE_DIR && makensis installer.nsi

cd $OPENSCADDIR

INSTALLFILE=$RELEASE_DIR/openscad_setup.exe

if [ -e $INSTALLFILE ]; then
	echo "Build complete. Install file ready: $INSTALLFILE"
else
	echo "Build failed. Sorry."
fi

