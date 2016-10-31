# Environment variables which can be set to specify library locations:
# MPFRDIR
# BOOSTDIR
# CGALDIR
# OPENCSGDIR
# OPENSCAD_LIBRARIES
#
# qmake Variables to define the installation:
#
#   PREFIX defines the base installation folder
#
# Please see the 'Building' sections of the OpenSCAD user manual 
# for updated tips & workarounds.
#
# http://en.wikibooks.org/wiki/OpenSCAD_User_Manual

OSNAME=$$system(uname -o)
contains(OSNAME,Msys) {
  # only use 'release' on windows, because qmake on win uses two makefiles
  # Makefile.release and Makefile.debug, the debug is broken b/c qcsintilla
  CONFIG=release
}

message("If you're building a Stable Release, please use CONFIG=deploy")
CONFIG+=experimental
DEFINES += ENABLE_EXPERIMENTAL
macx: {
  ICON = icons/icon-nightly.icns
}
deploy {
  CONFIG-=experimental
  DEFINES-=ENABLE_EXPERIMENTAL
  macx: {
    ICON = $$PWD/icons/OpenSCAD.icns
    CONFIG += sparkle
    QMAKE_RPATHDIR = @executable_path/../Frameworks
  }
}

mxetarget=$$(MXE_TARGET)
!isEmpty(mxetarget) {
  mxeabi=$$(MXE_TARGET)
  contains(mxeabi,shared) {
    CONFIG += mingw-cross-env-shared
  } else {
    CONFIG += mingw-cross-env
  }
}

TEMPLATE = app
INCLUDEPATH += src
DEPENDPATH += src

OPENSCAD_LIBDIR = $$(OPENSCAD_LIBRARIES)

!isEmpty(OPENSCAD_LIBDIR) {
  QMAKE_INCDIR = $$OPENSCAD_LIBDIR/include
  QMAKE_LIBDIR = $$OPENSCAD_LIBDIR/lib
}

macx {
  TARGET = OpenSCAD
  QMAKE_INFO_PLIST = Info.plist
  APP_RESOURCES.path = Contents/Resources
  APP_RESOURCES.files = OpenSCAD.sdef dsa_pub.pem icons/SCAD.icns
  QMAKE_BUNDLE_DATA += APP_RESOURCES
  LIBS += -framework Cocoa -framework ApplicationServices
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
}
else {	
  TARGET = openscad
}
win* {
  RC_FILE = openscad_win32.rc
  QMAKE_CXXFLAGS += -DNOGDI
}
contains(OSNAME,Msys): {
  QMAKE_CXXFLAGS -= -pipe # ctrl-c doesn't like pipes
}

RESOURCES = openscad.qrc

OBJECTS_DIR = objects
MOC_DIR = objects
UI_DIR = objects
RCC_DIR = objects
INCLUDEPATH += objects

macx:CONFIG += mdi

CONFIG += c++11
CONFIG += cgal
CONFIG += opencsg
CONFIG += boost
CONFIG += gettext
CONFIG += scintilla

include(cgal.pri)
include(opencsg.pri)
include(opengl.pri)
include(boost.pri)
include(gettext.pri)
include(sparkle.pri)
include(scintilla.pri)
include(c++11.pri)

CONFIG += link_pkgconfig
PKGCONFIG += eigen3 glew fontconfig freetype2 harfbuzz glib-2.0 libxml-2.0
contains(OSNAME,Msys): {
  PKGCONFIG += Qt5Core Qt5OpenGL Qt5Gui Qt5Concurrent
  CONFIG += moc opengl
}

QT += widgets core gui concurrent

# OPENSCAD_VERSION   format: yyyy.mm.dd.gitcommit
# format for stable release: yyyy.mm-patchlevel
# note - VERSION is a qmake keyword, do not use
isEmpty(OPENSCAD_VERSION) {
  datecmd=date
  contains(OSNAME,Msys): datecmd=$$(MINGW_PREFIX)/../usr/bin/date
  YMD=$$system($$datecmd "+%Y.%m.%d")
  COMMIT=$$system(git log -1 --pretty=format:"%h")
  !isEmpty(COMMIT): COMMIT=".$$COMMIT"
  OPENSCAD_VERSION=$$YMD$$COMMIT
}
DEFINES += OPENSCAD_VERSION=$$OPENSCAD_VERSION

# PKGNAME is for packaging and installation. 
# For ordinary dev build it is the full version. For Stable Release, it
# is simply 'openscad'.
# example as a pathname: /usr/local/share/openscad-2016.01.20.f43f/locale
OPENSCAD_PKGNAME = openscad-$$OPENSCAD_VERSION
deploy: $$OPENSCAD_PKGNAME = openscad

# mingw has to come after other items so OBJECT_DIRS will work properly

CONFIG(mingw-cross-env)|CONFIG(mingw-cross-env-shared): {
  QMAKE_CXXFLAGS += -fpermissive
  WINSTACKSIZE = 8388608 # 8MB # github issue 116
  QMAKE_CXXFLAGS += -Wl,--stack,$$WINSTACKSIZE
  LIBS += -Wl,--stack,$$WINSTACKSIZE
  QMAKE_DEL_FILE = rm -f
}

# Qt5 removed access to the QMAKE_UIC variable, the following
# way works for both Qt4 and Qt5
load(uic)
uic.commands += -tr _

FORMS   += src/MainWindow.ui \
           src/Preferences.ui \
           src/OpenCSGWarningDialog.ui \
           src/AboutDialog.ui \
           src/FontListDialog.ui \
           src/ProgressWidget.ui \
           src/launchingscreen.ui \
           src/LibraryInfoDialog.ui

LEXSOURCES += src/lexer.l
YACCSOURCES += src/parser.y
contains(OSNAME,Msys): CONFIG += yacc lex

HEADERS += src/AST.h \
           src/ModuleInstantiation.h \
           src/Package.h \
           src/Assignment.h \
           src/expression.h \
           src/function.h \
           src/module.h \           
           src/UserModule.h

SOURCES += src/AST.cc \
           src/ModuleInstantiation.cc \
           src/expr.cc \
           src/function.cc \
           src/module.cc \
           src/UserModule.cc

HEADERS += \
           src/ProgressWidget.h \
           src/parsersettings.h \
           src/renderer.h \
           src/settings.h \
           src/rendersettings.h \
           src/colormap.h \
           src/ThrownTogetherRenderer.h \
           src/CGAL_OGL_Polyhedron.h \
           src/OGL_helper.h \
           src/QGLView.h \
           src/GLView.h \
           src/MainWindow.h \
           src/OpenSCADApp.h \
           src/WindowManager.h \
           src/Preferences.h \
           src/OpenCSGWarningDialog.h \
           src/AboutDialog.h \
           src/FontListDialog.h \
           src/FontListTableView.h \
           src/GroupModule.h \
           src/FileModule.h \
           src/builtin.h \
           src/calc.h \
           src/context.h \
           src/modcontext.h \
           src/evalcontext.h \
           src/csgops.h \
           src/CSGTreeNormalizer.h \
           src/CSGTreeEvaluator.h \
           src/dxfdata.h \
           src/dxfdim.h \
           src/export.h \
           src/stackcheck.h \
           src/exceptions.h \
           src/grid.h \
           src/hash.h \
           src/highlighter.h \
           src/localscope.h \
           src/feature.h \
           src/node.h \
           src/csgnode.h \
           src/offsetnode.h \
           src/linearextrudenode.h \
           src/rotateextrudenode.h \
           src/projectionnode.h \
           src/cgaladvnode.h \
           src/importnode.h \
           src/import.h \
           src/transformnode.h \
           src/colornode.h \
           src/rendernode.h \
           src/textnode.h \
           src/openscad.h \
           src/handle_dep.h \
           src/Geometry.h \
           src/Polygon2d.h \
           src/clipper-utils.h \
           src/GeometryUtils.h \
           src/polyset-utils.h \
           src/polyset.h \
           src/printutils.h \
           src/fileutils.h \
           src/value.h \
           src/progress.h \
           src/editor.h \
           src/NodeVisitor.h \
           src/state.h \
           src/nodecache.h \
           src/nodedumper.h \
           src/ModuleCache.h \
           src/GeometryCache.h \
           src/GeometryEvaluator.h \
           src/Tree.h \
           src/DrawingCallback.h \
           src/FreetypeRenderer.h \
           src/FontCache.h \
           src/memory.h \
           src/linalg.h \
           src/Camera.h \
           src/system-gl.h \
           src/stl-utils.h \
           src/boost-utils.h \
           src/LibraryInfo.h \
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
           src/Dock.h \
           src/AutoUpdater.h \
           src/launchingscreen.h \
           src/LibraryInfoDialog.h

SOURCES += \
           src/libsvg/libsvg.cc \
           src/libsvg/circle.cc \
           src/libsvg/ellipse.cc \
           src/libsvg/line.cc \
           src/libsvg/polygon.cc \
           src/libsvg/polyline.cc \
           src/libsvg/rect.cc \
           src/libsvg/group.cc \
           src/libsvg/svgpage.cc \
           src/libsvg/path.cc \
           src/libsvg/shape.cc \
           src/libsvg/transformation.cc \
           src/libsvg/util.cc \
           \
           src/ProgressWidget.cc \
           src/linalg.cc \
           src/Camera.cc \
           src/handle_dep.cc \
           src/value.cc \
           src/stackcheck.cc \
           src/func.cc \
           src/localscope.cc \
           src/feature.cc \
           src/node.cc \
           src/context.cc \
           src/modcontext.cc \
           src/evalcontext.cc \
           src/csgnode.cc \
           src/CSGTreeNormalizer.cc \
           src/CSGTreeEvaluator.cc \
           src/Geometry.cc \
           src/Polygon2d.cc \
           src/clipper-utils.cc \
           src/polyset-utils.cc \
           src/GeometryUtils.cc \
           src/polyset.cc \
           src/polyset-gl.cc \
           src/csgops.cc \
           src/transform.cc \
           src/color.cc \
           src/primitives.cc \
           src/projection.cc \
           src/cgaladv.cc \
           src/surface.cc \
           src/control.cc \
           src/render.cc \
           src/text.cc \
           src/dxfdata.cc \
           src/dxfdim.cc \
           src/offset.cc \
           src/linearextrude.cc \
           src/rotateextrude.cc \
           src/printutils.cc \
           src/fileutils.cc \
           src/progress.cc \
           src/parsersettings.cc \
           src/stl-utils.cc \
           src/boost-utils.cc \
           src/PlatformUtils.cc \
           src/LibraryInfo.cc \
           \
           src/nodedumper.cc \
           src/NodeVisitor.cc \
           src/GeometryEvaluator.cc \
           src/ModuleCache.cc \
           src/GeometryCache.cc \
           src/Tree.cc \
	   src/DrawingCallback.cc \
	   src/FreetypeRenderer.cc \
	   src/FontCache.cc \
           \
           src/settings.cc \
           src/rendersettings.cc \
           src/highlighter.cc \
           src/Preferences.cc \
           src/OpenCSGWarningDialog.cc \
           src/editor.cc \
           src/GLView.cc \
           src/QGLView.cc \
           src/AutoUpdater.cc \
           \
           src/grid.cc \
           src/hash.cc \
           src/GroupModule.cc \
           src/FileModule.cc \
           src/builtin.cc \
           src/calc.cc \
           src/export.cc \
           src/export_stl.cc \
           src/export_amf.cc \
           src/export_off.cc \
           src/export_dxf.cc \
           src/export_svg.cc \
           src/export_nef.cc \
           src/export_png.cc \
           src/import.cc \
           src/import_stl.cc \
           src/import_off.cc \
           src/import_svg.cc \
           src/renderer.cc \
           src/colormap.cc \
           src/ThrownTogetherRenderer.cc \
           src/svg.cc \
           src/OffscreenView.cc \
           src/fbo.cc \
           src/system-gl.cc \
           src/imageutils.cc \
           src/lodepng.cpp \
           \
           src/openscad.cc \
           src/mainwin.cc \
           src/OpenSCADApp.cc \
           src/WindowManager.cc \
           src/UIUtils.cc \
           src/Dock.cc \
           src/FontListDialog.cc \
           src/FontListTableView.cc \
           src/launchingscreen.cc \
           src/LibraryInfoDialog.cc

# ClipperLib
SOURCES += src/polyclipping/clipper.cpp
HEADERS += src/polyclipping/clipper.hpp

# libtess2
INCLUDEPATH += src/libtess2/Include
SOURCES += src/libtess2/Source/bucketalloc.c \
           src/libtess2/Source/dict.c \
           src/libtess2/Source/geom.c \
           src/libtess2/Source/mesh.c \
           src/libtess2/Source/priorityq.c \
           src/libtess2/Source/sweep.c \
           src/libtess2/Source/tess.c
HEADERS += src/libtess2/Include/tesselator.h \
           src/libtess2/Source/bucketalloc.h \
           src/libtess2/Source/dict.h \
           src/libtess2/Source/geom.h \
           src/libtess2/Source/mesh.h \
           src/libtess2/Source/priorityq.h \
           src/libtess2/Source/sweep.h \
           src/libtess2/Source/tess.h

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
           src/Reindexer.h \
           src/CGALCache.h \
           src/CGALRenderer.h \
           src/CGAL_Nef_polyhedron.h \
           src/CGAL_Nef3_workaround.h \
           src/convex_hull_3_bugfix.h \
           src/cgalworker.h \
           src/Polygon2d-CGAL.h

SOURCES += src/cgalutils.cc \
           src/cgalutils-applyops.cc \
           src/cgalutils-project.cc \
           src/cgalutils-tess.cc \
           src/cgalutils-polyhedron.cc \
           src/CGALCache.cc \
           src/CGALRenderer.cc \
           src/CGAL_Nef_polyhedron.cc \
           src/cgalworker.cc \
           src/Polygon2d-CGAL.cc
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
  HEADERS += src/findversion.h
  SOURCES += src/PlatformUtils-win.cc
}

isEmpty(PREFIX):PREFIX = /usr/local

target.path = $$PREFIX/bin/
INSTALLS += target

# Run translation update scripts as last step after linking the target
POST_LINK_CMD = "$$_PRO_FILE_PWD_/scripts/translation-make.sh"
win32 {
  # on MSYS2, handle spaces in pathnames (ex if username is "Emmy Noether")
  contains(OSNAME,Msys) {
    POST_LINK_CMD = ___QUOTE___"$$POST_LINK_CMD"___QUOTE___
    POST_LINK_CMD = $$replace(POST_LINK_CMD,"___QUOTE___","\"")
    POST_LINK_CMD = $$shell_path($$POST_LINK_CMD)
  }
}
QMAKE_POST_LINK += $$POST_LINK_CMD

# Create install targets for the languages defined in LINGUAS
LINGUAS = $$cat($$_PRO_FILE_PWD_/locale/LINGUAS)
LOCALE_PREFIX = "$$PREFIX/share/$$OPENSCAD_PKGNAME/locale"
for(language, LINGUAS) {
  catalogdir = $$_PRO_FILE_PWD_/locale/$$language/LC_MESSAGES
  exists($$_PRO_FILE_PWD_/locale/$${language}.po) {
    # Use .extra and copy manually as the source path might not exist,
    # e.g. on a clean checkout. In that case qmake would not create
    # the needed targets in the generated Makefile.
    translation_path = translation_$${language}.path
    translation_extra = translation_$${language}.extra
    translation_depends = translation_$${language}.depends
    $$translation_path = $$LOCALE_PREFIX/$$language/LC_MESSAGES/
    $$translation_extra = cp -f $${catalogdir}/openscad.mo \"\$(INSTALL_ROOT)$$LOCALE_PREFIX/$$language/LC_MESSAGES/openscad.mo\"
    $$translation_depends = $$_PRO_FILE_PWD_/locale/$${language}.po
    INSTALLS += translation_$$language
  }
}

examples.path = "$$PREFIX/share/$$OPENSCAD_PKGNAME/examples/"
examples.files = $$PWD/examples/*
INSTALLS += examples

libraries.path = "$$PREFIX/share/$$OPENSCAD_PKGNAME/libraries/"
libraries.files = $$PWD/libraries/*
INSTALLS += libraries

fonts.path = "$$PREFIX/share/$$OPENSCAD_PKGNAME/fonts/"
fonts.files = $$PWD/fonts/*
INSTALLS += fonts

colorschemes.path = "$$PREFIX/share/$$OPENSCAD_PKGNAME/color-schemes/"
colorschemes.files = $$PWD/color-schemes/*
INSTALLS += colorschemes

applications.path = $$PREFIX/share/applications
applications.extra = cat $$PWD/icons/openscad.desktop | sed -e \"'s/^Icon=openscad/Icon=$$OPENSCAD_PKGNAME/; s/^Version=1.0/Version=$$OPENSCAD_VERSION/; s/^Exec=openscad/Exec=$$OPENSCAD_PKGNAME/'\" > \"\$(INSTALL_ROOT)$${applications.path}/openscad.desktop\"
INSTALLS += applications

mimexml.path = $$PREFIX/share/mime/packages
mimexml.extra = cp -f $$PWD/icons/openscad.xml \"\$(INSTALL_ROOT)$${mimexml.path}/$$OPENSCAD_PKGNAME.xml\"
INSTALLS += mimexml

appdata.path = $$PREFIX/share/appdata
appdata.extra = cp -f $$PWD/openscad.appdata.xml \"\$(INSTALL_ROOT)$${appdata.path}/$$OPENSCAD_PKGNAME.appdata.xml\"
INSTALLS += appdata

OPENSCAD_PNGICON=openscad.png
deploy: OPENSCAD_PNGICON=openscad-nightly.png

icons.path = $$PREFIX/share/pixmaps
icons.extra = test -f $$PWD/icons/$$OPENSCAD_PNGICON && cp -f $$PWD/icons/$$OPENSCAD_PNGICON \"\$(INSTALL_ROOT)$${icons.path}/\"
INSTALLS += icons

man.path = $$PREFIX/share/man/man1
man.extra = cp -f $$PWD/doc/openscad.1 \"\$(INSTALL_ROOT)$${man.path}/$$OPENSCAD_PKGNAME.1\"
INSTALLS += man

contains(OSNAME,Msys) {
  !exists(objects/openscad_win32_res.o) {
    message("please ignore WARNING: Failure to find objects/openscad_win32_res.o and proceed to run make")
  }
}
