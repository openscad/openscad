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
  APP_RESOURCES.path = Contents/Resources
  APP_RESOURCES.files = OpenSCAD.sdef
  QMAKE_BUNDLE_DATA += APP_RESOURCES
  #CONFIG += x86 ppc
  LIBS += -framework Carbon
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
macx:CONFIG += mdi
CONFIG += cgal
CONFIG += opencsg
macx:CONFIG += progresswidget

mdi {
  # MDI needs an OpenCSG library that is compiled with OpenCSG-Reset-Hack.patch applied
  DEFINES += ENABLE_MDI
}

progresswidget {
  DEFINES += USE_PROGRESSWIDGET
  FORMS   += src/ProgressWidget.ui
  HEADERS += src/ProgressWidget.h
  SOURCES += src/ProgressWidget.cc
}

include(cgal.pri)
include(opencsg.pri)

# Optionally specify location of Eigen2 using the 
# EIGEN2DIR env. variable
EIGEN2_DIR = $$(EIGEN2DIR)
!isEmpty(EIGEN2_DIR) {
  INCLUDEPATH += $$EIGEN2_DIR
}
else {
  INCLUDEPATH += /usr/include/eigen2
}

!macx:QMAKE_CXXFLAGS_RELEASE = -O3 -march=pentium

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
           src/value.h \
           src/progress.h

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
           src/projection.cc \
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
           src/Preferences.cc \
           src/progress.cc

macx {
  HEADERS += src/AppleEvents.h \
             src/EventFilter.h
  SOURCES += src/AppleEvents.cc
}

target.path = /usr/local/bin/
INSTALLS += target
