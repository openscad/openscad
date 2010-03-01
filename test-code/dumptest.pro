DEFINES += OPENSCAD_VERSION=test
TEMPLATE = app

OBJECTS_DIR = objects
MOC_DIR = objects
UI_DIR = objects
RCC_DIR = objects
INCLUDEPATH += ../src

TARGET = dumptest
macx {
  CONFIG -= app_bundle
  LIBS += -framework Carbon
}

CONFIG += qt
QT += opengl

# Optionally specify location of Eigen2 using the 
# EIGEN2DIR env. variable
EIGEN2_DIR = $$(EIGEN2DIR)
!isEmpty(EIGEN2_DIR) {
  INCLUDEPATH += $$EIGEN2_DIR
}
else {
  INCLUDEPATH += /usr/include/eigen2
}

LEXSOURCES += ../src/lexer.l
YACCSOURCES += ../src/parser.y

HEADERS += ../src/builtin.h \
           ../src/cgal.h \
           ../src/context.h \
           ../src/csgterm.h \
           ../src/dxfdata.h \
           ../src/dxfdim.h \
           ../src/dxftess.h \
           ../src/export.h \
           ../src/expression.h \
           ../src/function.h \
           ../src/grid.h \
           ../src/module.h \
           ../src/node.h \
           ../src/openscad.h \
           ../src/polyset.h \
           ../src/printutils.h \
           ../src/value.h \
           ../src/progress.h

SOURCES += dumptest.cc \
           ../src/export.cc \
           ../src/value.cc \
           ../src/expr.cc \
           ../src/func.cc \
           ../src/module.cc \
           ../src/node.cc \
           ../src/context.cc \
           ../src/csgterm.cc \
           ../src/polyset.cc \
           ../src/csgops.cc \
           ../src/transform.cc \
           ../src/primitives.cc \
           ../src/projection.cc \
           ../src/cgaladv.cc \
           ../src/surface.cc \
           ../src/control.cc \
           ../src/render.cc \
           ../src/import.cc \
           ../src/dxfdata.cc \
           ../src/dxftess.cc \
           ../src/dxftess-glu.cc \
           ../src/dxftess-cgal.cc \
           ../src/dxfdim.cc \
           ../src/dxflinextrude.cc \
           ../src/dxfrotextrude.cc \
           ../src/printutils.cc \
           ../src/progress.cc
