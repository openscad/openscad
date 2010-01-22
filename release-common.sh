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
        CONFIG=mdi
        ZIP=zip
        ZIPARGS=-qr
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

echo "Creating directory structure..."
rm -rf openscad-$VERSION
rm -f openscad-$VERSION.zip
mkdir -p openscad-$VERSION/examples
cp examples/* openscad-$VERSION/examples/
chmod -R 644 openscad-$VERSION/examples/*

case $OS in
    MACOSX)
        mkdir OpenSCAD.app/Contents/Frameworks
        cp $OPENCSGDIR/lib/libopencsg.dylib OpenSCAD.app/Contents/Frameworks
        cp /opt/local/lib/libGLEW.1.5.1.dylib OpenSCAD.app/Contents/Frameworks
        cp /Library/Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL OpenSCAD.app/Contents/Frameworks
        cp /Library/Frameworks/QtGui.framework/Versions/4/QtGui OpenSCAD.app/Contents/Frameworks
        cp /Library/Frameworks/QtCore.framework/Versions/4/QtCore OpenSCAD.app/Contents/Frameworks
        install_name_tool -change libopencsg.1.dylib @executable_path/../Frameworks/libopencsg.dylib OpenSCAD.app/Contents/MacOS/openscad
        install_name_tool -change QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL OpenSCAD.app/Contents/MacOS/openscad
        install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui OpenSCAD.app/Contents/MacOS/openscad
        install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore OpenSCAD.app/Contents/MacOS/openscad
        install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui OpenSCAD.app/Contents/Frameworks/QtOpenGL 
        install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore OpenSCAD.app/Contents/Frameworks/QtOpenGL 
        install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore OpenSCAD.app/Contents/Frameworks/QtGui
        install_name_tool -change /opt/local/lib/libGLEW.1.5.1.dylib @executable_path/../Frameworks/libGLEW.1.5.1.dylib OpenSCAD.app/Contents/MacOS/openscad
        install_name_tool -id libopencsg.dylib OpenSCAD.app/Contents/Frameworks/libopencsg.dylib
        install_name_tool -change /opt/local/lib/libGLEW.1.5.1.dylib @executable_path/../Frameworks/libGLEW.1.5.1.dylib OpenSCAD.app/Contents/Frameworks/libopencsg.dylib
        install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui OpenSCAD.app/Contents/Frameworks/libopencsg.dylib
        install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore OpenSCAD.app/Contents/Frameworks/libopencsg.dylib
        install_name_tool -id libGLEW.1.5.1.dylib OpenSCAD.app/Contents/Frameworks/libGLEW.1.5.1.dylib
        mv OpenSCAD.app openscad-$VERSION
    ;;
    WIN)
        #package
        cp win32deps/* openscad-$VERSION
        cp $TARGET/openscad.exe openscad-$VERSION
        ;;
esac

echo "Creating archive.."
"$ZIP" $ZIPARGS openscad-$VERSION.zip openscad-$VERSION

rm -rf openscad-$VERSION

echo "Binary created: openscad-$VERSION.zip"
