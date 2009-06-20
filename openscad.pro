
CONFIG += qt
TEMPLATE = app

LEXSOURCES = lexer.l
YACCSOURCES = parser.y

HEADERS = openscad.h
SOURCES = openscad.cc value.cc expr.cc func.cc module.cc context.cc

