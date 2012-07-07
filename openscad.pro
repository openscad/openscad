# Environment variables which can be set to specify library locations:
#   MPIRDIR
#   MPFRDIR
#   BOOSTDIR
#   CGALDIR
#   EIGEN2DIR
#   GLEWDIR
#   OPENCSGDIR
#   OPENSCAD_LIBRARIES
#
# Please see the 'Buildling' sections of the OpenSCAD user manual 
# for updated tips & workarounds.
#
# http://en.wikibooks.org/wiki/OpenSCAD_User_Manual

isEmpty(QT_VERSION) {
  error("Please use qmake for Qt 4 (probably qmake-qt4)")
}

# Auto-include config_<variant>.pri if the VARIANT variable is give on the
# command-line, e.g. qmake VARIANT=mybuild
!isEmpty(VARIANT) {
  message("Variant: $${VARIANT}")
  exists(config_$${VARIANT}.pri) {
    message("Including config_$${VARIANT}.pri")
    include(config_$${VARIANT}.pri)
  }
}

# Populate VERSION, VERSION_YEAR, VERSION_MONTH, VERSION_DATE from system date
include(version.pri)

# for debugging link problems (use nmake -f Makefile.Release > log.txt)
win32 {
  # QMAKE_LFLAGS += -VERBOSE
}
debug: DEFINES += DEBUG

TEMPLATE = app

INCLUDEPATH += src
INCLUDEPATH += tests

# Handle custom library location.
# Used when manually installing 3rd party libraries
OPENSCAD_LIBDIR = $$(OPENSCAD_LIBRARIES)
!isEmpty(OPENSCAD_LIBDIR) {
  QMAKE_INCDIR_QT = $$OPENSCAD_LIBDIR/include $$QMAKE_INCDIR_QT 
  QMAKE_LIBDIR = $$OPENSCAD_LIBDIR/lib $$QMAKE_LIBDIR
}
else {
  macx {
    # Default to MacPorts on Mac OS X
    QMAKE_INCDIR = /opt/local/include
    QMAKE_LIBDIR = /opt/local/lib
  }
}

macx {
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

# see http://fedoraproject.org/wiki/UnderstandingDSOLinkChange
# and https://github.com/openscad/openscad/pull/119
# ( QT += opengl does not automatically link glu+X11 on some DSO systems. )
unix:!macx {
  QMAKE_LIBS_OPENGL *= -lGLU
  QMAKE_LIBS_OPENGL *= -lX11
}

netbsd* {
   LIBS += -L/usr/X11R7/lib
   QMAKE_LFLAGS += -Wl,-R/usr/X11R7/lib
   QMAKE_LFLAGS += -Wl,-R/usr/pkg/lib
   !isEmpty(OPENSCAD_LIBDIR) {
     QMAKE_LFLAGS += -Wl,-R$$OPENSCAD_LIBDIR/lib
   }
}

# See Dec 2011 OpenSCAD mailing list, re: CGAL/GCC bugs.
*g++* {
  QMAKE_CXXFLAGS *= -fno-strict-aliasing
}

CONFIG(mingw-cross-env) {
  include(mingw-cross-env.pri)
}

# Application configuration
macx:CONFIG += mdi
CONFIG += cgal
CONFIG += opencsg
CONFIG += boost
CONFIG += eigen2

#Uncomment the following line to enable QCodeEdit
#CONFIG += qcodeedit

mdi {
  DEFINES += ENABLE_MDI
}

# FIXME: This can be made default by now
CONFIG += progresswidget
progresswidget {
  DEFINES += USE_PROGRESSWIDGET
  FORMS   += src/ProgressWidget.ui
  HEADERS += src/ProgressWidget.h
  SOURCES += src/ProgressWidget.cc
}

include(common.pri)

win32 {
  FLEXSOURCES = src/lexer.l
  BISONSOURCES = src/parser.y
} else {
  LEXSOURCES += src/lexer.l
  YACCSOURCES += src/parser.y
}

RESOURCES = openscad.qrc

FORMS   += src/MainWindow.ui \
           src/Preferences.ui \
           src/OpenCSGWarningDialog.ui

HEADERS += src/parsersettings.h \
           src/renderer.h \
           src/rendersettings.h \
           src/ThrownTogetherRenderer.h \
           src/CGAL_renderer.h \
           src/OGL_helper.h \
           src/GLView.h \
           src/MainWindow.h \
           src/Preferences.h \
           src/OpenCSGWarningDialog.h \
           src/builtin.h \
           src/context.h \
           src/csgterm.h \
           src/csgtermnormalizer.h \
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
           src/linearextrudenode.h \
           src/rotateextrudenode.h \
           src/projectionnode.h \
           src/cgaladvnode.h \
           src/importnode.h \
           src/transformnode.h \
           src/colornode.h \
           src/rendernode.h \
           src/openscad.h \
           src/handle_dep.h \
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
           src/ModuleCache.h \
           src/PolySetCache.h \
           src/PolySetEvaluator.h \
           src/CSGTermEvaluator.h \
           src/Tree.h \
           src/mathc99.h \
           src/memory.h \
           src/linalg.h \
           src/system-gl.h \
           src/stl-utils.h \
           src/CGALRenderer.h \
           tests/OffscreenView.h \
           tests/OffscreenContext.h \
           tests/bboxhelp.h \
           tests/fbo.h \
           tests/lodepng.h \
           tests/imageutils.h \
           tests/system-gl.h

SOURCES += src/mathc99.cc \
	   src/linalg.cc \
           src/handle_dep.cc \
           src/value.cc \
           src/expr.cc \
           src/func.cc \
           src/module.cc \
           src/node.cc \
           src/context.cc \
           src/csgterm.cc \
           src/csgtermnormalizer.cc \
           src/polyset.cc \
           src/csgops.cc \
           src/transform.cc \
           src/color.cc \
           src/primitives.cc \
           src/projection.cc \
           src/cgaladv.cc \
           src/surface.cc \
           src/control.cc \
           src/render.cc \
           src/dxfdata.cc \
           src/dxfdim.cc \
           src/linearextrude.cc \
           src/rotateextrude.cc \
           src/printutils.cc \
           src/progress.cc \
           src/parsersettings.cc \
           \
           src/nodedumper.cc \
           src/traverser.cc \
           src/PolySetEvaluator.cc \
           src/ModuleCache.cc \
           src/PolySetCache.cc \
           src/Tree.cc \
           \
           src/rendersettings.cc \
           src/highlighter.cc \
           src/Preferences.cc \
           src/OpenCSGWarningDialog.cc \
           src/editor.cc \
           src/glview.cc \
           \
           tests/OffscreenView.cc \
           tests/fbo.cc \
           tests/lodepng.cpp \
           tests/imageutils.cc \
           tests/system-gl.cc \
           tests/bboxhelp.cc \
           \
           src/builtin.cc \
           src/export.cc \
           src/import.cc \
           src/renderer.cc \
           src/ThrownTogetherRenderer.cc \
           src/dxftess.cc \
           src/dxftess-glu.cc \
           src/dxftess-cgal.cc \
           src/CSGTermEvaluator.cc \
           \
           src/openscad.cc \
           src/mainwin.cc

opencsg {
  HEADERS += src/OpenCSGRenderer.h
  SOURCES += src/OpenCSGRenderer.cc
}

cgal {
HEADERS += src/cgal.h \
           src/cgalfwd.h \
           src/cgalutils.h \
           src/CGALEvaluator.h \
           src/CGALCache.h \
           src/PolySetCGALEvaluator.h \
           src/CGALRenderer.h \
           src/CGAL_Nef_polyhedron.h \
           src/cgalworker.h

SOURCES += src/cgalutils.cc \
           src/CGALEvaluator.cc \
           src/PolySetCGALEvaluator.cc \
           src/CGALCache.cc \
           src/CGALRenderer.cc \
           src/CGAL_Nef_polyhedron.cc \
           src/CGAL_Nef_polyhedron_DxfData.cc \
           src/cgaladv_minkowski2.cc \
           src/cgalworker.cc
}

macx {
  HEADERS += src/AppleEvents.h \
             src/EventFilter.h
  SOURCES += src/AppleEvents.cc \
             tests/OffscreenContext.mm
}

unix {
  SOURCES += tests/OffscreenContextGLX.cc
}

win32 {
  SOURCES += tests/OffscreenContextWGL.cc
}

isEmpty(PREFIX):PREFIX = /usr/local

target.path = $$PREFIX/bin/
INSTALLS += target

examples.path = $$PREFIX/share/openscad/examples/
examples.files = examples/*
INSTALLS += examples

libraries.path = $$PREFIX/share/openscad/libraries/
libraries.files = libraries/*
INSTALLS += libraries

applications.path = $$PREFIX/share/applications
applications.files = icons/openscad.desktop
INSTALLS += applications

icons.path = $$PREFIX/share/pixmaps
icons.files = icons/openscad.png
INSTALLS += icons
