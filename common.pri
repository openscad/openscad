OBJECTS_DIR = objects
MOC_DIR = objects
UI_DIR = objects
RCC_DIR = objects

# Handle custom library location.
# Used when manually installing 3rd party libraries
isEmpty(OPENSCAD_LIBDIR) OPENSCAD_LIBDIR = $$(OPENSCAD_LIBRARIES)
macx:isEmpty(OPENSCAD_LIBDIR) {
  exists(/opt/local) {
    #Default to MacPorts on Mac OS X
    message("Automatically searching for libraries in /opt/local. To override, use qmake OPENSCAD_LIBDIR=<prefix>")
    OPENSCAD_LIBDIR = /opt/local
  }
}
!isEmpty(OPENSCAD_LIBDIR) {
  QMAKE_INCDIR = $$OPENSCAD_LIBDIR/include
  QMAKE_LIBDIR = $$OPENSCAD_LIBDIR/lib
}


include(qt.pri)
include(win.pri)
include(flex.pri)
include(bison.pri)
include(cgal.pri)
include(opencsg.pri)
include(opengl.pri)
include(glew.pri)
include(eigen.pri)
include(boost.pri)
include(glib-2.0.pri)
include(gettext.pri)
include(libxml2.pri)
include(sparkle.pri)
include(harfbuzz.pri)
include(freetype.pri)
include(fontconfig.pri)
include(scintilla.pri)
include(c++11.pri)
include(libzip.pri)
