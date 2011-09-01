# Auto-include config_<variant>.pri if the VARIANT variable is give on the
# command-line, e.g. qmake VARIANT=mybuild
!isEmpty(VARIANT) {
  message("Variant: $${VARIANT}")
  exists(config_$${VARIANT}.pri) {
    message("Including config_$${VARIANT}.pri")
    include(config_$${VARIANT}.pri)
  }
}

win32 {
  isEmpty(VERSION) VERSION = $$system(date /t)
} else {
  isEmpty(VERSION) VERSION = $$system(date "+%Y.%m.%d")
}

#configure lex / yacc
win32 {
  include(flex.pri)
  include(bison.pri)
  FLEXSOURCES = src/lexer.l
  BISONSOURCES = src/parser.y
} else {
  LEXSOURCES += src/lexer.l
  YACCSOURCES += src/parser.y
}

#configure additional directories
win32 {
    INCLUDEPATH += $$(MPIRDIR)
    INCLUDEPATH += $$(MPFRDIR)
}

DEFINES += OPENSCAD_VERSION=$$VERSION
win32:DEFINES += _USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS YY_NO_UNISTD_H

#disable warning about too long decorated names
win32:QMAKE_CXXFLAGS += -wd4503


TEMPLATE = app
RESOURCES = openscad.qrc

OBJECTS_DIR = objects
MOC_DIR = objects
UI_DIR = objects
RCC_DIR = objects
INCLUDEPATH += src

macx {
  DEPLOYDIR = $$(MACOSX_DEPLOY_DIR)
  !isEmpty(DEPLOYDIR) {
    INCLUDEPATH += $$DEPLOYDIR/include
    LIBS += -L$$DEPLOYDIR/lib
  }
  # add CONFIG+=deploy to the qmake command-line to make a deployment build
  deploy {
    message("Building deployment version")
    CONFIG += x86 x86_64
  }

  TARGET = OpenSCAD
  ICON = icons/OpenSCAD.icns
  QMAKE_INFO_PLIST = Info.plist
  APP_RESOURCES.path = Contents/Resources
  APP_RESOURCES.files = OpenSCAD.sdef
  QMAKE_BUNDLE_DATA += APP_RESOURCES
  LIBS += -framework Carbon
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
macx:CONFIG += mdi
CONFIG += cgal
CONFIG += opencsg
CONFIG += progresswidget
CONFIG += boost

#Uncomment the following line to enable QCodeEdit
#CONFIG += qcodeedit

mdi {
  # MDI needs an OpenCSG library that is compiled with OpenCSG-Reset-Hack.patch applied
  DEFINES += ENABLE_MDI
}

progresswidget {
  DEFINES += USE_PROGRESSWIDGET
  FORMS   += src/ProgressWidget.ui
  HEADERS += src/ProgressWidget.h
  SOURCES += src/ProgressWidget.cc
}

include(cgal.pri)
include(opencsg.pri)
include(eigen2.pri)
include(boost.pri)

# Standard include path for misc external libs
#macx {
#  INCLUDEPATH += /opt/local/include
#}

# QMAKE_CFLAGS   += -pg
# QMAKE_CXXFLAGS += -pg
# QMAKE_LFLAGS   += -pg


FORMS   += src/MainWindow.ui \
           src/Preferences.ui

HEADERS += src/renderer.h \
           src/ThrownTogetherRenderer.h \
           src/CGAL_renderer.h \
           src/OGL_helper.h \
           src/GLView.h \
           src/MainWindow.h \
           src/Preferences.h \
           src/builtin.h \
           src/context.h \
           src/csgterm.h \
           src/dxfdata.h \
           src/dxfdim.h \
           src/dxftess.h \
           src/export.h \
           src/expression.h \
           src/function.h \
           src/grid.h \
           src/highlighter.h \
           src/module.h \
           src/node.h \
           src/csgnode.h \
           src/dxflinextrudenode.h \
           src/dxfrotextrudenode.h \
           src/projectionnode.h \
           src/importnode.h \
           src/transformnode.h \
           src/rendernode.h \
           src/openscad.h \
           src/polyset.h \
           src/printutils.h \
           src/value.h \
           src/progress.h \
           src/editor.h \
           src/visitor.h \
           src/state.h \
           src/traverser.h \
           src/nodecache.h \
           src/nodedumper.h \
           src/PolySetEvaluator.h \
           src/CSGTermEvaluator.h \
           src/myqhash.h \
           src/Tree.h \
           src/mathc99.h \
           src/stl-utils.h

SOURCES += src/openscad.cc \
           src/mainwin.cc \
           src/ThrownTogetherRenderer.cc \
           src/glview.cc \
           src/export.cc \
           src/value.cc \
           src/expr.cc \
           src/func.cc \
           src/module.cc \
           src/node.cc \
           src/context.cc \
           src/csgterm.cc \
           src/polyset.cc \
           src/csgops.cc \
           src/transform.cc \
           src/primitives.cc \
           src/projection.cc \
           src/cgaladv.cc \
           src/surface.cc \
           src/control.cc \
           src/render.cc \
           src/import.cc \
           src/dxfdata.cc \
           src/dxftess.cc \
           src/dxftess-glu.cc \
           src/dxftess-cgal.cc \
           src/dxfdim.cc \
           src/dxflinextrude.cc \
           src/dxfrotextrude.cc \
           src/highlighter.cc \
           src/printutils.cc \
           src/Preferences.cc \
           src/progress.cc \
           src/editor.cc \
           src/traverser.cc \
           src/nodedumper.cc \
           src/CSGTermEvaluator.cc \
           src/qhash.cc \
           src/Tree.cc \
	   src/mathc99.cc \
           src/PolySetEvaluator.cc

cgal {
HEADERS += src/cgal.h \
           src/cgalfwd.h \
           src/CGALEvaluator.h \
           src/PolySetCGALEvaluator.h \
           src/cgalrenderer.h \
           src/CGAL_Nef_polyhedron.h

SOURCES += src/CGALEvaluator.cc \
           src/PolySetCGALEvaluator.cc \
           src/cgalrenderer.cc \
           src/CGAL_Nef_polyhedron.cc \
           src/CGAL_Nef_polyhedron_DxfData.cc \
	   src/cgaladv_convexhull2.cc \
           src/cgaladv_minkowski2.cc
}

macx {
  HEADERS += src/AppleEvents.h \
             src/EventFilter.h
  SOURCES += src/AppleEvents.cc
}

target.path = /usr/local/bin/
INSTALLS += target

examples.path = /usr/local/share/openscad/examples/
examples.files = examples/*
INSTALLS += examples

libraries.path = /usr/local/share/openscad/libraries/
libraries.files = libraries/*
INSTALLS += libraries
