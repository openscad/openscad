# Environment variables which can be set to specify library locations:
#   MPIRDIR
#   MPFRDIR
#   BOOSTDIR
#   CGALDIR
#   EIGENDIR
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
win* {
  # QMAKE_LFLAGS += -VERBOSE
}
debug: DEFINES += DEBUG

TEMPLATE = app

INCLUDEPATH += src

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

# add CONFIG+=deploy to the qmake command-line to make a deployment build
deploy {
  message("Building deployment version")
  DEFINES += OPENSCAD_DEPLOY
  macx {
    CONFIG += x86 x86_64
    LIBS += -framework Sparkle
    HEADERS += src/SparkleAutoUpdater.h
    OBJECTIVE_SOURCES += src/SparkleAutoUpdater.mm
  }
}

macx {
  TARGET = OpenSCAD
  ICON = icons/OpenSCAD.icns
  QMAKE_INFO_PLIST = Info.plist
  APP_RESOURCES.path = Contents/Resources
  APP_RESOURCES.files = OpenSCAD.sdef dsa_pub.pem icons/SCAD.icns
  QMAKE_BUNDLE_DATA += APP_RESOURCES
  LIBS += -framework Cocoa

  # FIXME: Somehow, setting the deployment target to a lower version causes a
  # seldom crash in debug mode (e.g. the minkowski2-test):
  # frame #4: 0x00007fff8b7d5be5 libc++.1.dylib`std::runtime_error::~runtime_error() + 55
  # frame #5: 0x0000000100150df5 OpenSCAD`CGAL::Uncertain_conversion_exception::~Uncertain_conversion_exception(this=0x0000000105044488) + 21 at Uncertain.h:78
  # The reason for the crash appears to be linking with libgcc_s, 
  # but it's unclear what's really going on
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
}
else {
  TARGET = openscad
}

win* {
  RC_FILE = openscad_win32.rc
}

CONFIG += qt
QT += opengl

# see http://fedoraproject.org/wiki/UnderstandingDSOLinkChange
# and https://github.com/openscad/openscad/pull/119
# ( QT += opengl does not automatically link glu on some DSO systems. )
unix:!macx {
  QMAKE_LIBS_OPENGL *= -lGLU
  QMAKE_LIBS_OPENGL *= -lX11
}

netbsd* {
   QMAKE_LFLAGS += -L/usr/X11R7/lib
   QMAKE_LFLAGS += -Wl,-R/usr/X11R7/lib
   QMAKE_LFLAGS += -Wl,-R/usr/pkg/lib
   !clang: { QMAKE_CXXFLAGS += -std=c++0x }
   !isEmpty(OPENSCAD_LIBDIR) {
     QMAKE_CFLAGS = -I$$OPENSCAD_LIBDIR/include $$QMAKE_CFLAGS
     QMAKE_CXXFLAGS = -I$$OPENSCAD_LIBDIR/include $$QMAKE_CXXFLAGS
     QMAKE_LFLAGS = -L$$OPENSCAD_LIBDIR/lib $$QMAKE_LFLAGS
     QMAKE_LFLAGS = -Wl,-R$$OPENSCAD_LIBDIR/lib $$QMAKE_LFLAGS
   }
}

# Prevent LD_LIBRARY_PATH problems when running the openscad binary
# on systems where uni-build-dependencies.sh was used. 
# Will not affect 'normal' builds.
!isEmpty(OPENSCAD_LIBDIR) {
  unix:!macx {
    QMAKE_LFLAGS = -Wl,-R$$OPENSCAD_LIBDIR/lib $$QMAKE_LFLAGS
    # need /lib64 beause GLEW installs itself there on 64 bit machines
    QMAKE_LFLAGS = -Wl,-R$$OPENSCAD_LIBDIR/lib64 $$QMAKE_LFLAGS 
  }
}

# See Dec 2011 OpenSCAD mailing list, re: CGAL/GCC bugs.
*g++* {
  QMAKE_CXXFLAGS *= -fno-strict-aliasing
}

*clang* {
	# http://llvm.org/bugs/show_bug.cgi?id=9182
	QMAKE_CXXFLAGS_WARN_ON += -Wno-overloaded-virtual
	# disable enormous amount of warnings about CGAL / boost / etc
	QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
	QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable
	QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
	QMAKE_CXXFLAGS_WARN_ON += -Wno-c++11-extensions
	# might want to actually turn this on once in a while
	QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare
}

CONFIG(skip-version-check) {
  # force the use of outdated libraries
  DEFINES += OPENSCAD_SKIP_VERSION_CHECK
}

# Application configuration
macx:CONFIG += mdi
CONFIG += cgal
CONFIG += opencsg
CONFIG += boost
CONFIG += eigen

#Uncomment the following line to enable QCodeEdit
#CONFIG += qcodeedit

mdi {
  DEFINES += ENABLE_MDI
}

DEFINES += USE_PROGRESSWIDGET

include(common.pri)

# mingw has to come after other items so OBJECT_DIRS will work properly
CONFIG(mingw-cross-env) {
  include(mingw-cross-env.pri)
}

win* {
  FLEXSOURCES = src/lexer.l
  BISONSOURCES = src/parser.y
} else {
  LEXSOURCES += src/lexer.l
  YACCSOURCES += src/parser.y
}

RESOURCES = openscad.qrc

FORMS   += src/MainWindow.ui \
           src/Preferences.ui \
           src/OpenCSGWarningDialog.ui \
           src/AboutDialog.ui \
           src/ProgressWidget.ui

HEADERS += src/typedefs.h \
           src/version_check.h \
           src/ProgressWidget.h \
           src/parsersettings.h \
           src/renderer.h \
           src/rendersettings.h \
           src/ThrownTogetherRenderer.h \
           src/CGAL_renderer.h \
           src/OGL_helper.h \
           src/QGLView.h \
           src/GLView.h \
           src/MainWindow.h \
           src/Preferences.h \
           src/OpenCSGWarningDialog.h \
           src/AboutDialog.h \
           src/builtin.h \
           src/context.h \
           src/modcontext.h \
           src/evalcontext.h \
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
           src/localscope.h \
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
           src/fileutils.h \
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
           src/Camera.h \
           src/system-gl.h \
           src/stl-utils.h \
           src/boost-utils.h \
           src/svg.h \
           \
           src/lodepng.h \
           src/OffscreenView.h \
           src/OffscreenContext.h \
           src/OffscreenContextAll.hpp \
           src/fbo.h \
           src/imageutils.h \
           src/system-gl.h \
           src/CsgInfo.h \
           \
           src/AutoUpdater.h

SOURCES += src/version_check.cc \
           src/ProgressWidget.cc \
           src/mathc99.cc \
           src/linalg.cc \
           src/handle_dep.cc \
           src/value.cc \
           src/expr.cc \
           src/func.cc \
           src/localscope.cc \
           src/module.cc \
           src/node.cc \
           src/context.cc \
           src/modcontext.cc \
           src/evalcontext.cc \
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
           src/fileutils.cc \
           src/progress.cc \
           src/parsersettings.cc \
           src/stl-utils.cc \
           src/boost-utils.cc \
           src/PlatformUtils.cc \
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
           src/GLView.cc \
           src/QGLView.cc \
           src/AutoUpdater.cc \
           \
           src/builtin.cc \
           src/export.cc \
           src/export_png.cc \
           src/import.cc \
           src/renderer.cc \
           src/ThrownTogetherRenderer.cc \
           src/dxftess.cc \
           src/dxftess-glu.cc \
           src/dxftess-cgal.cc \
           src/CSGTermEvaluator.cc \
           src/svg.cc \
           src/OffscreenView.cc \
           src/fbo.cc \
           src/system-gl.cc \
           src/imageutils.cc \
           src/lodepng.cpp \
           \
           src/openscad.cc \
           src/mainwin.cc

unix:!macx {
  SOURCES += src/imageutils-lodepng.cc
  SOURCES += src/OffscreenContextGLX.cc
}
macx {
  SOURCES += src/imageutils-macosx.cc
  OBJECTIVE_SOURCES += src/OffscreenContextCGL.mm
}
win* {
  SOURCES += src/imageutils-lodepng.cc
  SOURCES += src/OffscreenContextWGL.cc
}

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
             src/EventFilter.h \
             src/CocoaUtils.h
  SOURCES += src/AppleEvents.cc
  OBJECTIVE_SOURCES += src/CocoaUtils.mm \
                       src/PlatformUtils-mac.mm
}
unix:!macx {
  SOURCES += src/PlatformUtils-posix.cc
}
win* {
  SOURCES += src/PlatformUtils-win.cc
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

CONFIG(winconsole) {
  include(winconsole.pri)
}

