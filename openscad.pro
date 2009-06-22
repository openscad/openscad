
CONFIG += qt
TEMPLATE = app

LIBS += -lCGAL -lmpfr

LEXSOURCES += lexer.l
YACCSOURCES += parser.y

HEADERS += openscad.h
SOURCES += openscad.cc value.cc expr.cc func.cc module.cc context.cc
SOURCES += csg.cc trans.cc primitive.cc

