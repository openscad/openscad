isEmpty(VERSION) VERSION = $$system(date "+%Y.%m.%d")
DEFINES += OPENSCAD_VERSION=$$VERSION
TEMPLATE = app
RESOURCES = openscad.qrc

macx {
  TARGET = OpenSCAD
  ICON = OpenSCAD.icns
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
CONFIG += debug
# CONFIG += release
# CONFIG += mdi
CONFIG += cgal
CONFIG += opencsg

mdi {
  # MDI needs an OpenCSG library that is compiled with OpenCSG-Reset-Hack.patch applied
  DEFINES += ENABLE_MDI
}

cgal {
# Uncomment this to enable experimental CGAL tesselation
# DEFINES += CGAL_TESSELATE
  DEFINES += ENABLE_CGAL
  LIBS += -lCGAL
  macx {
    INCLUDEPATH += $(PWD)/../install/include /opt/local/include
    # The -L/usr/lib is to force the linker to use system libraries over MacPort libraries
    LIBS += -L/usr/lib -L$(PWD)/../install/lib -L/opt/local/lib /opt/local/lib/libgmp.a /opt/local/lib/libmpfr.a /opt/local/lib/libboost_thread-mt.a
  }
  else {
    LIBS += -lmpfr
  }
  win32:LIBS += -lboost_thread -lgmp
  QMAKE_CXXFLAGS += -frounding-math
}

opencsg {
  DEFINES += ENABLE_OPENCSG
  LIBS += -lopencsg
  unix:LIBS += -lGLEW
  win32:LIBS += -lglew32

  # Optionally specify location of OpenCSG using the 
  # OPENCSGDIR env. variable
  OPENCSG_DIR = $$(OPENCSGDIR)
  !isEmpty(OPENCSG_DIR) {
    INCLUDEPATH += $$OPENCSG_DIR/include
    LIBS += -L$$OPENCSG_DIR/lib
  }
  macx {
    # For glew
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib
  }
}

QMAKE_CXXFLAGS_RELEASE = -O3 -march=pentium
QMAKE_CXXFLAGS_DEBUG = -O0 -ggdb

# QMAKE_CFLAGS   += -pg
# QMAKE_CXXFLAGS += -pg
# QMAKE_LFLAGS   += -pg

LEXSOURCES += lexer.l
YACCSOURCES += parser.y

FORMS   += MainWindow.ui \
           Preferences.ui

HEADERS += openscad.h \
           MainWindow.h \
           Preferences.h \
           GLView.h \
           printutils.h \
           CGAL_renderer.h

macx: HEADERS += EventFilter.h

SOURCES += openscad.cc mainwin.cc glview.cc export.cc \
           value.cc expr.cc func.cc module.cc context.cc \
           csgterm.cc polyset.cc csgops.cc transform.cc \
           primitives.cc surface.cc control.cc render.cc \
           import.cc dxfdata.cc dxftess.cc dxftess-glu.cc \
           dxftess-cgal.cc dxfdim.cc \
           dxflinextrude.cc dxfrotextrude.cc highlighter.cc \
           printutils.cc nef2dxf.cc \
           Preferences.cc

target.path = /usr/local/bin/
INSTALLS += target
