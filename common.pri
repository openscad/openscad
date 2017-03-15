OBJECTS_DIR = objects
MOC_DIR = objects
UI_DIR = objects
RCC_DIR = objects

# Handle custom library location.
# Used when manually installing 3rd party libraries
isEmpty(OPENSCAD_LIBDIR) OPENSCAD_LIBDIR = $$(OPENSCAD_LIBRARIES)
macx:isEmpty(OPENSCAD_LIBDIR) {
  exists(/opt/local):exists(/usr/local/Cellar) {
    error("It seems you might have libraries in both /opt/local and /usr/local. Please specify which one to use with qmake OPENSCAD_LIBDIR=<prefix>")
  } else {
    exists(/opt/local) {
      #Default to MacPorts on Mac OS X
      message("Automatically searching for libraries in /opt/local. To override, use qmake OPENSCAD_LIBDIR=<prefix>")
      OPENSCAD_LIBDIR = /opt/local
    } else:exists(/usr/local/Cellar) {
      message("Automatically searching for libraries in /usr/local. To override, use qmake OPENSCAD_LIBDIR=<prefix>")
      OPENSCAD_LIBDIR = /usr/local
    }
  }
}
!isEmpty(OPENSCAD_LIBDIR) {
  QMAKE_INCDIR = $$OPENSCAD_LIBDIR/include
  QMAKE_LIBDIR = $$OPENSCAD_LIBDIR/lib
}


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
