#!/bin/sh
# WARNING: This script might only work with the authors setup...

mkdir openscad.app/Contents/Frameworks
cp ../OpenCSG-1.1.1/lib/libopencsg.dylib openscad.app/Contents/Frameworks
cp /opt/local/lib/libGLEW.1.5.1.dylib openscad.app/Contents/Frameworks
cp /Library/Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL openscad.app/Contents/Frameworks
cp /Library/Frameworks/QtGui.framework/Versions/4/QtGui openscad.app/Contents/Frameworks
cp /Library/Frameworks/QtCore.framework/Versions/4/QtCore openscad.app/Contents/Frameworks
install_name_tool -change libopencsg.1.dylib @executable_path/../Frameworks/libopencsg.dylib openscad.app/Contents/MacOS/openscad
install_name_tool -change QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL openscad.app/Contents/MacOS/openscad
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui openscad.app/Contents/MacOS/openscad
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore openscad.app/Contents/MacOS/openscad
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui openscad.app/Contents/Frameworks/QtOpenGL 
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore openscad.app/Contents/Frameworks/QtOpenGL 
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore openscad.app/Contents/Frameworks/QtGui
 install_name_tool -change /opt/local/lib/libGLEW.1.5.1.dylib @executable_path/../Frameworks/libGLEW.1.5.1.dylib openscad.app/Contents/MacOS/openscad
install_name_tool -change /opt/local/lib/libGLEW.1.5.1.dylib @executable_path/../Frameworks/libGLEW.1.5.1.dylib openscad.app/Contents/Frameworks/libopencsg.dylib

mkdir -p openscad/examples
cp examples/* openscad/examples/
chmod -R 644 openscad/examples/*
mv OpenSCAD.app openscad

