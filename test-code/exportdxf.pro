DEFINES += OPENSCAD_VERSION=test
TEMPLATE = app

OBJECTS_DIR = cgal-objects
MOC_DIR = cgal-objects
UI_DIR = cgal-objects
RCC_DIR = cgal-objects
INCLUDEPATH += ../src

macx {
  macx {
    DEPLOYDIR = $$(MACOSX_DEPLOY_DIR)
    !isEmpty(DEPLOYDIR) {
      INCLUDEPATH += $$DEPLOYDIR/include
      LIBS += -L$$DEPLOYDIR/lib
    }
  }
  CONFIG -= app_bundle
  LIBS += -framework Carbon
}

CONFIG += qt
QT += opengl
CONFIG += cgal

include(../cgal.pri)
include(../eigen2.pri)

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
           ../src/CGALRenderer.h \
           ../src/nodecache.h \
           ../src/importnode.h \
           ../src/state.h \
           ../src/PolySetRenderer.h \
           ../src/PolySetCGALRenderer.h \
           ../src/myqhash.h \
           ../src/Tree.h

SOURCES += exportdxf.cc \
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
           ../src/cgaladv_minkowski2.cc \
           ../src/cgaladv_minkowski3.cc \
           ../src/surface.cc \
           ../src/control.cc \
           ../src/render.cc \
           ../src/import.cc \
           ../src/dxfdata.cc \
           ../src/nef2dxf.cc \
           ../src/dxftess.cc \
           ../src/dxftess-glu.cc \
           ../src/dxftess-cgal.cc \
           ../src/dxfdim.cc \
           ../src/dxflinextrude.cc \
           ../src/dxfrotextrude.cc \
           ../src/printutils.cc \
           ../src/progress.cc \
           ../src/nodedumper.cc \
           ../src/CGALRenderer.cc \
           ../src/traverser.cc \
           ../src/PolySetRenderer.cc \
           ../src/PolySetCGALRenderer.cc \
           ../src/qhash.cc \
           ../src/Tree.cc
