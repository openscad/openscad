#!/bin/sh
#
# This script creates a binary release of OpenSCAD for Mac OS X.
# The script will create a file called openscad-<versionstring>.zip
# in the current directory.
# 
# Usage: release-win32.sh [-v <versionstring>]
#  -v   Version string (e.g. -v 2010.01)
#
# If no version string is given, todays date will be used (YYYY-MM-DD)
#

#used for windows
ZIP="/c/Program Files/7-Zip/7z.exe"
ZIPARGS="a -tzip"

printUsage()
{
  echo "Usage: $0 -v <versionstring> -t <buildtarget>"
  echo
  echo "  Example: $0 -v 2010.01 -t release"
}

OS=OSX
if test "`uname -o`" == "Msys"; then
    OS=WIN
fi

echo "detected OS= $OS"

while getopts 'v:' c
do
  case $c in
    v) VERSION=$OPTARG;;
    b) TARGET=$OPTARG;;
  esac
done

if test -z "$VERSION"; then
    VERSION=`date "+%Y.%m.%d"`
fi

if test -z "$TARGET"; then
    TARGET=release
fi

echo "Building openscad-$VERSION $CONFIGURATION..."

case $OS in
    OSX) 
        CONFIG = mdi;;
    WIN) 
        unset CONFIG
        export QTDIR=/c/devmingw/qt2009.03
        export QTMAKESPEC=win32-g++
        export PATH=$PATH:/c/devmingw/qt2009.03/bin:/c/devmingw/qt2009.03/qt/bin
        ;;
esac

qmake VERSION=$VERSION CONFIG+=$CONFIG
make clean
if test $OS == WIN; then
    #if the following files are missing their tried removal stops the build process on msys
    touch -t 200012121010 parser_yacc.h parser_yacc.cpp lexer_lex.cpp
fi

make -j2 $TARGET

echo "Preparing executable..."

echo "Creating directory structure..."
rm -rf openscad-$VERSION
rm -f openscad-$VERSION.zip
mkdir -p openscad-$VERSION/examples
cp examples/* openscad-$VERSION/examples/

case $OS in
    OSX) ;;
    WIN)
        #package
        cp win32deps/* openscad-$VERSION
        cp $TARGET/openscad.exe openscad-$VERSION
        ;;
esac

echo "Creating directory structure..."
case $OS in
    OSX) ;;
    WIN)
        "$ZIP" $ZIPARGS openscad-$VERSION.zip openscad-$VERSION
        ;;
esac

rm -rf openscad-$VERSION

echo "binary created: openscad-$VERSION.zip"
