
CONFIG += qt debug
TEMPLATE = app

DEFINES += "ENABLE_CGAL=1"
LIBS += -lCGAL -lmpfr

DEFINES += "ENABLE_OPENCSG=1"
LIBS += -lopencsg -lGLEW

LEXSOURCES += lexer.l
YACCSOURCES += parser.y

HEADERS += openscad.h
SOURCES += openscad.cc mainwin.cc glview.cc
SOURCES += value.cc expr.cc func.cc module.cc context.cc
SOURCES += csgterm.cc polyset.cc csgops.cc transform.cc
SOURCES += primitives.cc control.cc render.cc
SOURCES += dxfdata.cc dxflinextrude.cc dxfrotextrude.cc

QMAKE_CXXFLAGS += -O0
// QMAKE_CXXFLAGS += -O3 -march=pentium

QT += opengl

target.path = /usr/local/bin/
INSTALLS += target

