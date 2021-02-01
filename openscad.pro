# Environment variables which can be set to specify library locations:
# MPIRDIR
# MPFRDIR
# BOOSTDIR
# CGALDIR
# EIGENDIR
# GLEWDIR
# OPENCSGDIR
# OPENSCAD_LIBRARIES
#
# qmake Variables to define the installation:
#
#   PREFIX defines the base installation folder
#
#   SUFFIX defines an optional suffix for the binary and the
#   resource folder. E.g. using SUFFIX=-nightly will name the
#   resulting binary openscad-nightly.
#
# Please see the 'Building' sections of the OpenSCAD user manual
# for updated tips & workarounds.
#
# https://en.wikibooks.org/wiki/OpenSCAD_User_Manual

include(defaults.pri)

# Local settings are read from local.pri
exists(local.pri): include(local.pri)

# Auto-include config_<variant>.pri if the VARIANT variable is given on the
# command-line, e.g. qmake VARIANT=mybuild
!isEmpty(VARIANT) {
  message("Variant: $${VARIANT}")
  exists(config_$${VARIANT}.pri) {
    message("Including config_$${VARIANT}.pri")
    include(config_$${VARIANT}.pri)
  }
}

debug {
  experimental {
    message("Building experimental debug version")
  }
  else {
    message("If you're building a development binary, consider adding CONFIG+=experimental")
  }
}
  
# If VERSION is not set, populate VERSION, VERSION_YEAR, VERSION_MONTH from system date
include(version.pri)

debug: DEFINES += DEBUG

TEMPLATE = app

INCLUDEPATH += src
DEPENDPATH += src

# add CONFIG+=deploy to the qmake command-line to make a deployment build
deploy {
  message("Building deployment version")
  DEFINES += OPENSCAD_DEPLOY
  macx: {
    CONFIG += sparkle
    OBJECTIVE_SOURCES += src/SparkleAutoUpdater.mm
    QMAKE_RPATHDIR = @executable_path/../Frameworks
  }
}
snapshot {
  DEFINES += OPENSCAD_SNAPSHOT
}
# add CONFIG+=idprefix to the qmake command-line to debug node ID's in csg output
idprefix {
  DEFINES += IDPREFIX
  message("Setting IDPREFIX for csg debugging")
  warning("Setting IDPREFIX will negatively affect cache hits")
}  
macx {
  TARGET = OpenSCAD
}
else {
  TARGET = openscad$${SUFFIX}
}
FULLNAME = openscad$${SUFFIX}
APPLICATIONID = org.openscad.OpenSCAD
!isEmpty(SUFFIX): DEFINES += INSTALL_SUFFIX="\"\\\"$${SUFFIX}\\\"\""

macx {
  snapshot {
    ICON = icons/icon-nightly.icns
  }
  else {
    ICON = icons/OpenSCAD.icns
  }
  QMAKE_INFO_PLIST = Info.plist
  APP_RESOURCES.path = Contents/Resources
  APP_RESOURCES.files = OpenSCAD.sdef dsa_pub.pem icons/SCAD.icns
  QMAKE_BUNDLE_DATA += APP_RESOURCES
  LIBS += -framework Cocoa -framework ApplicationServices
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
}

# Set same stack size for the linker and #define used in PlatformUtils.h
STACKSIZE = 8388608 # 8MB # github issue 116
QMAKE_CXXFLAGS += -DSTACKSIZE=$$STACKSIZE
DEFINES += STACKSIZE=$$STACKSIZE

win* {
  RC_FILE = openscad_win32.rc
  QMAKE_CXXFLAGS += -DNOGDI
  QMAKE_LFLAGS += -Wl,--stack,$$STACKSIZE
}

mingw* {
  # needed to prevent compilation error on MSYS2:
  # as.exe: objects/cgalutils.o: too many sections (76541)
  # using -Wa,-mbig-obj did not help
  debug: QMAKE_CXXFLAGS += -O1
}

CONFIG += qt object_parallel_to_source
QT += widgets concurrent multimedia network
CONFIG += scintilla

netbsd* {
   QMAKE_LFLAGS += -L/usr/X11R7/lib
   QMAKE_LFLAGS += -Wl,-R/usr/X11R7/lib
   QMAKE_LFLAGS += -Wl,-R/usr/pkg/lib
   # FIXME: Can the lines below be removed in favour of the OPENSCAD_LIBDIR handling above?
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
    # need /lib64 because GLEW installs itself there on 64 bit machines
    QMAKE_LFLAGS = -Wl,-R$$OPENSCAD_LIBDIR/lib64 $$QMAKE_LFLAGS
  }
}

# See Dec 2011 OpenSCAD mailing list, re: CGAL/GCC bugs.
*g++* {
  QMAKE_CXXFLAGS *= -fno-strict-aliasing
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedefs # ignored before 4.8

  # Disable attributes warnings on MSYS/MXE due to gcc bug spamming the logs: Issue #2771
  win* | CONFIG(mingw-cross-env)|CONFIG(mingw-cross-env-shared) {
    QMAKE_CXXFLAGS += -Wno-attributes
  }
}

*clang* {
  # http://llvm.org/bugs/show_bug.cgi?id=9182
  QMAKE_CXXFLAGS_WARN_ON += -Wno-overloaded-virtual
  # disable enormous amount of warnings about CGAL / boost / etc
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
  # gettext
  QMAKE_CXXFLAGS_WARN_ON += -Wno-format-security
  # might want to actually turn this on once in a while
  QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare
}

skip-version-check {
  # force the use of outdated libraries
  DEFINES += OPENSCAD_SKIP_VERSION_CHECK
}

isEmpty(PKG_CONFIG):PKG_CONFIG = pkg-config

# Application configuration
CONFIG += c++std
CONFIG += cgal
CONFIG += opencsg
CONFIG += glew
CONFIG += boost
CONFIG += eigen
CONFIG += glib-2.0
CONFIG += harfbuzz
CONFIG += freetype
CONFIG += fontconfig
CONFIG += lib3mf
CONFIG += gettext
CONFIG += libxml2
CONFIG += libzip
CONFIG += hidapi
CONFIG += spnav
CONFIG += double-conversion
CONFIG += cairo

# Make experimental features available
experimental {
  DEFINES += ENABLE_EXPERIMENTAL
}

nogui {
  DEFINES += OPENSCAD_NOGUI
}

mdi {
  DEFINES += ENABLE_MDI
}

system("ccache -V >/dev/null 2>/dev/null") {
  CONFIG += ccache
  message("Using ccache")
}

include(common.pri)

# mingw has to come after other items so OBJECT_DIRS will work properly
CONFIG(mingw-cross-env)|CONFIG(mingw-cross-env-shared) {
  include(mingw-cross-env.pri)
}

RESOURCES = openscad.qrc

# Qt5 removed access to the QMAKE_UIC variable, the following
# way works for both Qt4 and Qt5
load(uic)
uic.commands += -tr q_

FORMS   += src/MainWindow.ui \
           src/ErrorLog.ui \
           src/Preferences.ui \
           src/OpenCSGWarningDialog.ui \
           src/AboutDialog.ui \
           src/FontListDialog.ui \
           src/PrintInitDialog.ui \
           src/ProgressWidget.ui \
           src/launchingscreen.ui \
           src/LibraryInfoDialog.ui \
           src/Console.ui \
           src/parameter/ParameterWidget.ui \
           src/parameter/ParameterEntryWidget.ui \
           src/input/ButtonConfigWidget.ui \
           src/input/AxisConfigWidget.ui

# AST nodes
FLEXSOURCES += src/lexer.l 
BISONSOURCES += src/parser.y

HEADERS += src/AST.h \
           src/ModuleInstantiation.h \
           src/Package.h \
           src/Assignment.h \
           src/expression.h \
           src/function.h \
           src/module.h \           
           src/UserModule.h \

SOURCES += src/AST.cc \
           src/ModuleInstantiation.cc \
           src/Assignment.cc \
           src/export_pdf.cc \
           src/expr.cc \
           src/function.cc \
           src/module.cc \
           src/UserModule.cc \
           src/annotation.cc

# Comment parser
FLEXSOURCES += src/comment_lexer.l
BISONSOURCES += src/comment_parser.y

HEADERS += src/version_check.h \
           src/version_helper.h \
           src/ProgressWidget.h \
           src/parsersettings.h \
           src/renderer.h \
	   src/VertexArray.h \
           src/VBORenderer.h \
           src/settings.h \
           src/rendersettings.h \
           src/colormap.h \
           src/ThrownTogetherRenderer.h \
           src/CGAL_OGL_Polyhedron.h \
           src/QGLView.h \
           src/GLView.h \
           src/MainWindow.h \
           src/tabmanager.h \
           src/tabwidget.h \
           src/OpenSCADApp.h \
           src/WindowManager.h \
           src/initConfig.h \
           src/Preferences.h \
           src/SettingsWriter.h \
           src/OpenCSGWarningDialog.h \
           src/AboutDialog.h \
           src/FontListDialog.h \
           src/FontListTableView.h \
           src/GroupModule.h \
           src/FileModule.h \
           src/StatCache.h \
           src/scadapi.h \
           src/builtin.h \
           src/calc.h \
           src/context.h \
           src/builtincontext.h \
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
           src/version.h \
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
           src/boost-utils.h \
           src/LibraryInfo.h \
           src/RenderStatistic.h \
           src/svg.h \
           src/mouseselector.h \
           \
           src/OffscreenView.h \
           src/OffscreenContext.h \
           src/OffscreenContextAll.hpp \
           src/fbo.h \
           src/imageutils.h \
           src/system-gl.h \
           src/CsgInfo.h \
           \
           src/Dock.h \
           src/Console.h \
           src/ErrorLog.h \
           src/AutoUpdater.h \
           src/launchingscreen.h \
           src/LibraryInfoDialog.h \
           \
           src/comment.h\
           \
           src/parameter/ParameterWidget.h \
           src/parameter/parameterobject.h \
           src/parameter/parameterextractor.h \
           src/parameter/parametervirtualwidget.h \
           src/parameter/parameterspinbox.h \
           src/parameter/parametercombobox.h \
           src/parameter/parameterslider.h \
           src/parameter/parametercheckbox.h \
           src/parameter/parametertext.h \
           src/parameter/parametervector.h \
           src/parameter/groupwidget.h \
           src/parameter/parameterset.h \
           src/parameter/ignoreWheelWhenNotFocused.h \
           src/QWordSearchField.h \
           src/QSettingsCached.h \
           src/input/InputDriver.h \
           src/input/InputEventMapper.h \
           src/input/InputDriverManager.h \
           src/input/AxisConfigWidget.h \
           src/input/ButtonConfigWidget.h \
           src/input/WheelIgnorer.h

SOURCES += \
           src/libsvg/libsvg.cc \
           src/libsvg/circle.cc \
           src/libsvg/ellipse.cc \
           src/libsvg/line.cc \
           src/libsvg/text.cc \
           src/libsvg/tspan.cc \
           src/libsvg/data.cc \
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
           src/version_check.cc

SOURCES += \
           src/ProgressWidget.cc \
           src/linalg.cc \
           src/Camera.cc \
           src/handle_dep.cc \
           src/value.cc \
           src/degree_trig.cc \
           src/func.cc \
           src/localscope.cc \
           src/feature.cc \
           src/node.cc \
           src/context.cc \
           src/builtincontext.cc \
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
           src/boost-utils.cc \
           src/PlatformUtils.cc \
           src/LibraryInfo.cc \
           src/RenderStatistic.cc \
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
           src/initConfig.cc \
           src/Preferences.cc \
           src/SettingsWriter.cc \
           src/OpenCSGWarningDialog.cc \
           src/editor.cc \
           src/GLView.cc \
           src/QGLView.cc \
           src/AutoUpdater.cc \
           \
           src/hash.cc \
           src/GroupModule.cc \
           src/FileModule.cc \
           src/StatCache.cc \
           src/scadapi.cc \
           src/builtin.cc \
           src/calc.cc \
           src/export.cc \
           src/export_stl.cc \
           src/export_amf.cc \
           src/export_3mf.cc \
           src/export_off.cc \
           src/export_dxf.cc \
           src/export_svg.cc \
           src/export_nef.cc \
           src/export_png.cc \
           src/import.cc \
           src/import_stl.cc \
           src/import_off.cc \
           src/import_svg.cc \
           src/import_amf.cc \
           src/import_3mf.cc \
           src/renderer.cc \
	   src/VertexArray.cc \
           src/VBORenderer.cc \
           src/colormap.cc \
           src/ThrownTogetherRenderer.cc \
           src/svg.cc \
           src/OffscreenView.cc \
           src/fbo.cc \
           src/system-gl.cc \
           src/imageutils.cc \
           \
           src/version.cc \
           src/openscad.cc \
           src/mainwin.cc \
           src/tabmanager.cc \
           src/tabwidget.cc \
           src/OpenSCADApp.cc \
           src/WindowManager.cc \
           src/UIUtils.cc \
           src/Dock.cc \
           src/Console.cc \
           src/ErrorLog.cc \
           src/FontListDialog.cc \
           src/FontListTableView.cc \
           src/launchingscreen.cc \
           src/LibraryInfoDialog.cc\
           \
           src/comment.cpp \
           src/mouseselector.cc \
           \
           src/parameter/ParameterWidget.cc\
           src/parameter/parameterobject.cpp \
           src/parameter/parameterextractor.cpp \
           src/parameter/parameterspinbox.cpp \
           src/parameter/parametercombobox.cpp \
           src/parameter/parameterslider.cpp \
           src/parameter/parametercheckbox.cpp \
           src/parameter/parametertext.cpp \
           src/parameter/parametervector.cpp \
           src/parameter/groupwidget.cpp \
           src/parameter/parameterset.cpp \
           src/parameter/parametervirtualwidget.cpp \
           src/parameter/ignoreWheelWhenNotFocused.cpp \
           src/QWordSearchField.cc\
           src/QSettingsCached.cc \
           \
           src/input/InputDriver.cc \
           src/input/InputEventMapper.cc \
           src/input/InputDriverManager.cc \
           src/input/AxisConfigWidget.cc \
           src/input/ButtonConfigWidget.cc \
           src/input/WheelIgnorer.cc

# CGAL
HEADERS += src/ext/CGAL/OGL_helper.h \
           src/ext/CGAL/CGAL_workaround_Mark_bounded_volumes.h

# LodePNG
SOURCES += src/ext/lodepng/lodepng.cpp
HEADERS += src/ext/lodepng/lodepng.h
           
# ClipperLib
SOURCES += src/ext/polyclipping/clipper.cpp
HEADERS += src/ext/polyclipping/clipper.hpp

# libtess2
INCLUDEPATH += src/ext/libtess2/Include
SOURCES += src/ext/libtess2/Source/bucketalloc.c \
           src/ext/libtess2/Source/dict.c \
           src/ext/libtess2/Source/geom.c \
           src/ext/libtess2/Source/mesh.c \
           src/ext/libtess2/Source/priorityq.c \
           src/ext/libtess2/Source/sweep.c \
           src/ext/libtess2/Source/tess.c
HEADERS += src/ext/libtess2/Include/tesselator.h \
           src/ext/libtess2/Source/bucketalloc.h \
           src/ext/libtess2/Source/dict.h \
           src/ext/libtess2/Source/geom.h \
           src/ext/libtess2/Source/mesh.h \
           src/ext/libtess2/Source/priorityq.h \
           src/ext/libtess2/Source/sweep.h \
           src/ext/libtess2/Source/tess.h

has_qt5 {
  HEADERS += src/Network.h src/NetworkSignal.h src/PrintService.h src/OctoPrint.h src/PrintInitDialog.h
  SOURCES += src/PrintService.cc src/OctoPrint.cc src/PrintInitDialog.cc
}

has_qt5:unix:!macx {
  QT += dbus
  DEFINES += ENABLE_DBUS
  DBUS_ADAPTORS += org.openscad.OpenSCAD.xml
  DBUS_INTERFACES += org.openscad.OpenSCAD.xml

  HEADERS += src/input/DBusInputDriver.h
  SOURCES += src/input/DBusInputDriver.cc
}

linux: {
  DEFINES += ENABLE_JOYSTICK

  HEADERS += src/input/JoystickInputDriver.h
  SOURCES += src/input/JoystickInputDriver.cc
}

!lessThan(QT_MAJOR_VERSION, 5) {
  qtHaveModule(gamepad) {
    QT += gamepad
    DEFINES += ENABLE_QGAMEPAD
    HEADERS += src/input/QGamepadInputDriver.h
    SOURCES += src/input/QGamepadInputDriver.cc
  }
}

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
           src/cgalutils.h \
           src/Reindexer.h \
           src/CGALCache.h \
           src/CGALRenderer.h \
           src/CGAL_Nef_polyhedron.h \
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
           src/Polygon2d-CGAL.cc \
           src/import_nef.cc
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
QMAKE_POST_LINK += "'$$PWD/scripts/translation-make.sh'"

# Create install targets for the languages defined in LINGUAS
LINGUAS = $$cat(locale/LINGUAS)
LOCALE_PREFIX = "$$PREFIX/share/$${FULLNAME}/locale"
for(language, LINGUAS) {
  catalogdir = locale/$$language/LC_MESSAGES
  exists(locale/$${language}.po) {
    # Use .extra and copy manually as the source path might not exist,
    # e.g. on a clean checkout. In that case qmake would not create
    # the needed targets in the generated Makefile.
    translation_path = translation_$${language}.path
    translation_extra = translation_$${language}.extra
    translation_depends = translation_$${language}.depends
    $$translation_path = $$LOCALE_PREFIX/$$language/LC_MESSAGES/
    $$translation_extra = cp -f $${catalogdir}/openscad.mo \"\$(INSTALL_ROOT)$$LOCALE_PREFIX/$$language/LC_MESSAGES/openscad.mo\"
    $$translation_depends = locale/$${language}.po
    INSTALLS += translation_$$language
  }
}

examples.path = "$$PREFIX/share/$${FULLNAME}/examples/"
examples.files = examples/*
INSTALLS += examples

libraries.path = "$$PREFIX/share/$${FULLNAME}/libraries/"
libraries.files = libraries/*
INSTALLS += libraries

fonts.path = "$$PREFIX/share/$${FULLNAME}/fonts/"
fonts.files = fonts/*
INSTALLS += fonts

colorschemes.path = "$$PREFIX/share/$${FULLNAME}/color-schemes/"
colorschemes.files = color-schemes/*
INSTALLS += colorschemes

templates.path = "$$PREFIX/share/$${FULLNAME}/templates/"
templates.files = templates/*
INSTALLS += templates

applications.path = $$PREFIX/share/applications
applications.extra = mkdir -p \"\$(INSTALL_ROOT)$${applications.path}\" && cat icons/openscad.desktop | sed -e \"'s/^Icon=openscad/Icon=$${FULLNAME}/; s/^Exec=openscad/Exec=$${FULLNAME}/'\" > \"\$(INSTALL_ROOT)$${applications.path}/$${FULLNAME}.desktop\"
INSTALLS += applications

mimexml.path = $$PREFIX/share/mime/packages
mimexml.extra = cp -f icons/openscad.xml \"\$(INSTALL_ROOT)$${mimexml.path}/$${FULLNAME}.xml\"
INSTALLS += mimexml

appdata.path = $$PREFIX/share/metainfo
appdata.extra = mkdir -p \"\$(INSTALL_ROOT)$${appdata.path}\" && cat openscad.appdata.xml | sed -e \"'s/$${APPLICATIONID}/$${APPLICATIONID}$${SUFFIX}/; s/openscad.desktop/openscad$${SUFFIX}.desktop/; s/openscad.png/openscad$${SUFFIX}.png/'\" > \"\$(INSTALL_ROOT)$${appdata.path}/$${APPLICATIONID}$${SUFFIX}.appdata.xml\"
INSTALLS += appdata

icon48.path = $$PREFIX/share/icons/hicolor/48x48/apps
icon48.extra = test -f icons/$${FULLNAME}-48.png && cp -f icons/$${FULLNAME}-48.png \"\$(INSTALL_ROOT)$${icon48.path}/$${FULLNAME}.png\" || cp -f icons/openscad-48.png \"\$(INSTALL_ROOT)$${icon48.path}/$${FULLNAME}.png\"
icon64.path = $$PREFIX/share/icons/hicolor/64x64/apps
icon64.extra = test -f icons/$${FULLNAME}-64.png && cp -f icons/$${FULLNAME}-64.png \"\$(INSTALL_ROOT)$${icon64.path}/$${FULLNAME}.png\" || cp -f icons/openscad-64.png \"\$(INSTALL_ROOT)$${icon64.path}/$${FULLNAME}.png\"
icon128.path = $$PREFIX/share/icons/hicolor/128x128/apps
icon128.extra = test -f icons/$${FULLNAME}-128.png && cp -f icons/$${FULLNAME}-128.png \"\$(INSTALL_ROOT)$${icon128.path}/$${FULLNAME}.png\" || cp -f icons/openscad-128.png \"\$(INSTALL_ROOT)$${icon128.path}/$${FULLNAME}.png\"
icon256.path = $$PREFIX/share/icons/hicolor/256x256/apps
icon256.extra = test -f icons/$${FULLNAME}-256.png && cp -f icons/$${FULLNAME}-256.png \"\$(INSTALL_ROOT)$${icon256.path}/$${FULLNAME}.png\" || cp -f icons/openscad-256.png \"\$(INSTALL_ROOT)$${icon256.path}/$${FULLNAME}.png\"
icon512.path = $$PREFIX/share/icons/hicolor/512x512/apps
icon512.extra = test -f icons/$${FULLNAME}-512.png && cp -f icons/$${FULLNAME}-512.png \"\$(INSTALL_ROOT)$${icon512.path}/$${FULLNAME}.png\" || cp -f icons/openscad-512.png \"\$(INSTALL_ROOT)$${icon512.path}/$${FULLNAME}.png\"
INSTALLS += icon48 icon64 icon128 icon256 icon512

man.path = $$PREFIX/share/man/man1
man.extra = cp -f doc/openscad.1 \"\$(INSTALL_ROOT)$${man.path}/$${FULLNAME}.1\"
INSTALLS += man

info: {
    include(info.pri)
}

DISTFILES += \
    sounds/complete.wav
