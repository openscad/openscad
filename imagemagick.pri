imagemagick {

  CONFIG(mingw-cross-env) {
    IMAGEMAGICK_INCLUDEPATH = mingw-cross-env/include/ImageMagick
  }

  # Optionally specify location of ImageMagick using the 
  # OPENSCAD_LIBRARIES env. variable
  isEmpty(IMAGEMAGICK_INCLUDEPATH) {
    OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
    !isEmpty(OPENSCAD_LIBRARIES_DIR) {
      exists($$OPENSCAD_LIBRARIES_DIR/include/ImageMagick) {
        IMAGEMAGICK_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/ImageMagick
      }
    }
  }

  # Optionally specify location of ImageMagick using the 
  # IMAGEMAGICKDIR env. variable
  isEmpty(IMAGEMAGICK_INCLUDEPATH) {
    IMAGEMAGICK_DIR = $$(IMAGEMAGICKDIR)
    !isEmpty(IMAGEMAGICK_DIR) { 
      IMAGEMAGICK_INCLUDEPATH = $$IMAGEMAGICK_DIR
      message("IMAGEMAGICK location: $$IMAGEMAGICK_INCLUDEPATH")
    }
  }

  isEmpty(IMAGEMAGICK_INCLUDEPATH) {
    freebsd-g++: IMAGEMAGICK_INCLUDEPATH = /usr/local/include/ImageMagick
    macx: IMAGEMAGICK_INCLUDEPATH = /opt/local/include/ImageMagick
    linux*|hurd*: IMAGEMAGICK_INCLUDEPATH = /usr/include/ImageMagick
    netbsd*: IMAGEMAGICK_INCLUDEPATH = /usr/pkg/include/ImageMagick
  }

  # imagemagick being under 'include/ImageMagick' needs special prepending
#  QMAKE_INCDIR_QT = $$IMAGEMAGICK_INCLUDEPATH $$IMAGEMAGICK_INCLUDEPATH/Magick++ $$QMAKE_INCDIR_QT
  QMAKE_INCDIR_QT = $$IMAGEMAGICK_INCLUDEPATH $$QMAKE_INCDIR_QT
#  INCLUDEPATH += $$IMAGEMAGICK_INCLUDEPATH
  LIBS += -lMagick++

}
