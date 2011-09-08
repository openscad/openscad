#!/bin/sh
#
# This script creates a binary release of OpenSCAD.
# This should work under Mac OS X and Windows (msys). Linux support pending.
# The script will create a file called openscad-<versionstring>.zip
# in the current directory.
# 
# Usage: release-common.sh [-v <versionstring>]
#  -v   Version string (e.g. -v 2010.01)
#
# If no version string is given, todays date will be used (YYYY-MM-DD)
# If no make target is given, release will be used on Windows, none one Mac OS X
#

printUsage()
{
  echo "Usage: $0 -v <versionstring>
  echo
  echo "  Example: $0 -v 2010.01
}

if [[ $OSTYPE =~ "darwin" ]]; then
  OS=MACOSX
elif [[ $OSTYPE == "msys" ]]; then
  OS=WIN
fi

echo "Detected OS: $OS"

while getopts 'v:' c
do
  case $c in
    v) VERSION=$OPTARG;;
  esac
done

if test -z "$VERSION"; then
    VERSION=`date "+%Y.%m.%d"`
fi

echo "Building openscad-$VERSION $CONFIGURATION..."

case $OS in
    MACOSX) 
        CONFIG=deploy
        TARGET=
        ;;
    WIN) 
        unset CONFIG
        export QTDIR=/c/devmingw/qt2009.03
        export QTMAKESPEC=win32-g++
        export PATH=$PATH:/c/devmingw/qt2009.03/bin:/c/devmingw/qt2009.03/qt/bin
        ZIP="/c/Program Files/7-Zip/7z.exe"
        ZIPARGS="a -tzip"
        TARGET=release
        ;;
esac

qmake VERSION=$VERSION CONFIG+=$CONFIG CONFIG-=debug openscad.pro
make -s clean
case $OS in
    MACOSX) 
        rm -rf OpenSCAD.app
        ;;
    WIN)
            #if the following files are missing their tried removal stops the build process on msys
        touch -t 200012121010 parser_yacc.h parser_yacc.cpp lexer_lex.cpp
        ;;
esac

make -j2 $TARGET

if [[ $? != 0 ]]; then
  echo "Error building OpenSCAD. Aborting."
  exit 1
fi

echo "Creating directory structure..."

case $OS in
    MACOSX)
        EXAMPLESDIR=OpenSCAD.app/Contents/Resources/examples
        LIBRARYDIR=OpenSCAD.app/Contents/Resources/libraries
    ;;
    *)
        EXAMPLESDIR=openscad-$VERSION/examples/
        LIBRARYDIR=openscad-$VERSION/libraries/
        rm -rf openscad-$VERSION
        mkdir openscad-$VERSION
    ;;
esac

if [ -n $EXAMPLESDIR ]; then
  echo $EXAMPLESDIR
  mkdir -p $EXAMPLESDIR
  cp examples/* $EXAMPLESDIR
  chmod -R 644 $EXAMPLESDIR/*
fi
if [ -n $LIBRARYDIR ]; then
  echo $LIBRARYDIR
  mkdir -p $LIBRARYDIR
  cp -R libraries/* $LIBRARYDIR
  chmod -R u=rwx,go=r,+X $LIBRARYDIR/*
  rm -rf `find $LIBRARYDIR -name ".git"`
fi

echo "Creating archive.."

case $OS in
    MACOSX)
        macdeployqt OpenSCAD.app -dmg -no-strip
        mv OpenSCAD.dmg OpenSCAD-$VERSION.dmg
        hdiutil internet-enable -yes -quiet OpenSCAD-$VERSION.dmg
        echo "Binary created: OpenSCAD-$VERSION.dmg"
    ;;
    WIN)
        #package
        cp win32deps/* openscad-$VERSION
        cp $TARGET/openscad.exe openscad-$VERSION
        rm -f openscad-$VERSION.zip
        "$ZIP" $ZIPARGS openscad-$VERSION.zip openscad-$VERSION
        rm -rf openscad-$VERSION
        echo "Binary created: openscad-$VERSION.zip"
        ;;
esac
