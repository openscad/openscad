isEmpty(VERSION) VERSION = $$system(date "+%Y.%m.%d")
DEFINES += OPENSCAD_VERSION=$$VERSION
TEMPLATE = app
RESOURCES = openscad.qrc

OBJECTS_DIR = objects
MOC_DIR = objects
UI_DIR = objects
RCC_DIR = objects
INCLUDEPATH += src

macx {
  TARGET = OpenSCAD
  ICON = icons/OpenSCAD.icns
  QMAKE_INFO_PLIST = Info.plist
  #CONFIG += x86 ppc
}
else {
  TARGET = openscad
}

win32 {
  RC_FILE = openscad_win32.rc
}

CONFIG += qt
QT += opengl

# Application configuration
# CONFIG += mdi
CONFIG += cgal
CONFIG += opencsg

mdi {
  # MDI needs an OpenCSG library that is compiled with OpenCSG-Reset-Hack.patch applied
  DEFINES += ENABLE_MDI
}

include(cgal.pri)
include(opencsg.pri)

QMAKE_CXXFLAGS_RELEASE = -O3 -march=pentium
QMAKE_CXXFLAGS_DEBUG = -O0 -ggdb

# QMAKE_CFLAGS   += -pg
# QMAKE_CXXFLAGS += -pg
# QMAKE_LFLAGS   += -pg



LEXSOURCES += src/lexer.l
YACCSOURCES += src/parser.y

FORMS   += src/MainWindow.ui \
           src/Preferences.ui

HEADERS += src/CGAL_renderer.h \
           src/GLView.h \
           src/MainWindow.h \
           src/Preferences.h \
           src/builtin.h \
           src/cgal.h \
           src/context.h \
           src/csgterm.h \
           src/dxfdata.h \
           src/dxfdim.h \
           src/dxftess.h \
           src/export.h \
           src/expression.h \
           src/function.h \
           src/grid.h \
           src/highlighter.h \
           src/module.h \
           src/node.h \
           src/openscad.h \
           src/polyset.h \
           src/printutils.h \
           src/value.h

SOURCES += src/openscad.cc \
           src/mainwin.cc \
           src/glview.cc \
           src/export.cc \
           src/value.cc \
           src/expr.cc \
           src/func.cc \
           src/module.cc \
           src/node.cc \
           src/context.cc \
           src/csgterm.cc \
           src/polyset.cc \
           src/csgops.cc \
           src/transform.cc \
           src/primitives.cc \
           src/surface.cc \
           src/control.cc \
           src/render.cc \
           src/import.cc \
           src/dxfdata.cc \
           src/dxftess.cc \
           src/dxftess-glu.cc \
           src/dxftess-cgal.cc \
           src/dxfdim.cc \
           src/dxflinextrude.cc \
           src/dxfrotextrude.cc \
           src/highlighter.cc \
           src/printutils.cc \
           src/nef2dxf.cc \
           src/Preferences.cc

target.path = /usr/local/bin/
INSTALLS += target
