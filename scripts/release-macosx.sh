#!/bin/sh
#
# This script creates a binary release of OpenSCAD for Mac OS X.
# The script will create a file called openscad-<versionstring>.zip
# in the current directory.
# 
# Usage: makedmg.sh [-v <versionstring>]
#  -v   Version string (e.g. -v 2010.01)
#
# If no version string is given, todays date will be used (YYYY-MM-DD)
#
printUsage()
{
  echo "Usage: $0 -v <versionstring>"
  echo
  echo "  Example: $0 -v 2010.01"
}

while getopts 'v:' c
do
  case $c in
    v) VERSION=$OPTARG;;
  esac
done

if test -z "$VERSION"; then
  VERSION=`date "+%Y.%m.%d"`
fi

echo "Building openscad-$VERSION..."
export OPENCSGDIR=$PWD/../OpenCSG-1.2.0
qmake VERSION=$VERSION CONFIG+=mdi openscad.pro
make clean
make -j2
echo "Preparing executable.."
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

echo "Creating directory structure.."
rm -rf openscad-$VERSION
mkdir -p openscad-$VERSION/examples
cp examples/* openscad-$VERSION/examples/
chmod -R 644 openscad-$VERSION/examples/*
mv OpenSCAD.app openscad-$VERSION

echo "Creating archive.."
zip -qr openscad-$VERSION.zip openscad-$VERSION
echo "Mac OS X binary created: openscad-$VERSION.zip"
