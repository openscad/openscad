
CONFIG += qt
TEMPLATE = app

LIBS += -lCGAL

LEXSOURCES += lexer.l
YACCSOURCES += parser.y

HEADERS += openscad.h
SOURCES += openscad.cc value.cc expr.cc func.cc module.cc context.cc
SOURCES += union.cc difference.cc intersect.cc
SOURCES += trans.cc
SOURCES += cube.cc

