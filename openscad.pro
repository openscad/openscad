
CONFIG += qt debug
TEMPLATE = app

DEFINES += "ENABLE_CGAL=1"
LIBS += -lCGAL -lmpfr

DEFINES += "ENABLE_OPENCSG=1"
LIBS += -lopencsg -lGLEW -lglut

LEXSOURCES += lexer.l
YACCSOURCES += parser.y

HEADERS += openscad.h
SOURCES += openscad.cc mainwin.cc glview.cc
SOURCES += value.cc expr.cc func.cc module.cc context.cc
SOURCES += csgterm.cc polyset.cc csg.cc trans.cc
SOURCES += primitive.cc

QT += opengl

