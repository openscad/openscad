DEFINES += OPENSCAD_VERSION=test
TEMPLATE = app

OBJECTS_DIR = objects
MOC_DIR = objects
UI_DIR = objects
RCC_DIR = objects
INCLUDEPATH += ../src

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
  macx {
    INCLUDEPATH += /opt/local/include/eigen2
  }
  else {
    INCLUDEPATH += /usr/include/eigen2
  }
}

LEXSOURCES += ../src/lexer.l
YACCSOURCES += ../src/parser.y

HEADERS += ../src/builtin.h \
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
           ../src/dxflinextrudenode.h \
           ../src/dxfrotextrudenode.h \
           ../src/projectionnode.h \
           ../src/importnode.h \
           ../src/csgnode.h \
           ../src/transformnode.h \
           ../src/rendernode.h \
           ../src/openscad.h \
           ../src/polyset.h \
           ../src/printutils.h \
           ../src/value.h \
           ../src/progress.h \
           ../src/traverser.h \
           ../src/csgnode.h \
           ../src/visitor.h \
           ../src/nodedumper.h \
           ../src/nodecache.h \
           ../src/importnode.h \
           ../src/state.h \
           ../src/PolySetRenderer.h \
           ../src/Tree.h \
           ../src/myqhash.h \
           ../src/CSGTermRenderer.h

SOURCES += csgtexttest.cc \
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
           ../src/progress.cc \
           ../src/nodedumper.cc \
           ../src/traverser.cc \
           ../src/PolySetRenderer.cc \
           ../src/Tree.cc \
           ../src/qhash.cc \
           ../src/CSGTermRenderer.cc
