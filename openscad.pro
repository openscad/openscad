isEmpty(VERSION) VERSION = $$system(date "+%Y.%m.%d")
DEFINES += OPENSCAD_VERSION=$$VERSION
TEMPLATE = app

macx {
  TARGET = OpenSCAD
  ICON = OpenSCAD.icns
  QMAKE_INFO_PLIST = Info.plist
  #CONFIG += x86 ppc
}
else {
  TARGET = openscad
}

CONFIG += qt
QT += opengl

# Application configuration
CONFIG += debug
# CONFIG += release
#CONFIG += mdi
CONFIG += cgal
CONFIG += opencsg

mdi {
  # MDI needs an OpenCSG library that is compiled with OpenCSG-Reset-Hack.patch applied
  DEFINES += ENABLE_MDI
}

cgal {
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
  LIBS += -L/opt/local/lib -lopencsg
  unix:LIBS += -lGLEW
  win32:LIBS += -lglew32
  macx {
    INCLUDEPATH += $(PWD)/../OpenCSG-1.1.1/include /opt/local/include
    LIBS += -L$(PWD)/../OpenCSG-1.1.1/lib
  }
}

QMAKE_CXXFLAGS_RELEASE = -O3 -march=pentium
QMAKE_CXXFLAGS_DEBUG = -O0 -ggdb

LEXSOURCES += lexer.l
YACCSOURCES += parser.y

FORMS   += MainWindow.ui

HEADERS += openscad.h \
           MainWindow.h \
           GLView.h \
           printutils.h

macx: HEADERS += EventFilter.h

SOURCES += openscad.cc mainwin.cc glview.cc export.cc \
           value.cc expr.cc func.cc module.cc context.cc \
           csgterm.cc polyset.cc csgops.cc transform.cc \
           primitives.cc surface.cc control.cc render.cc \
           import.cc dxfdata.cc dxftess.cc dxfdim.cc \
           dxflinextrude.cc dxfrotextrude.cc highlighter.cc \
           printutils.cc

target.path = /usr/local/bin/
INSTALLS += target
