#!/bin/sh
# WARNING: This script might only work with the authors setup...

VERSION=2009.11

echo "Preparing executable.."
mkdir OpenSCAD.app/Contents/Frameworks
cp ../OpenCSG-1.1.1/lib/libopencsg.dylib OpenSCAD.app/Contents/Frameworks
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
install_name_tool -change /opt/local/lib/libGLEW.1.5.1.dylib @executable_path/../Frameworks/libGLEW.1.5.1.dylib OpenSCAD.app/Contents/Frameworks/libopencsg.dylib
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui OpenSCAD.app/Contents/Frameworks/libopencsg.dylib
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore OpenSCAD.app/Contents/Frameworks/libopencsg.dylib

echo "Creating directory structure.."
rm -rf openscad-$VERSION
mkdir -p openscad-$VERSION/examples
cp examples/* openscad-$VERSION/examples/
chmod -R 644 openscad-$VERSION/examples/*
mv OpenSCAD.app openscad-$VERSION

echo "Creating archive.."
zip -qr openscad-$VERSION.zip openscad-$VERSION
echo "Mac OS X binary created: openscad-$VERSION.zip"
