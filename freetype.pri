# Detect freetype2, then use this priority list to determine
# which library to use:
#
# Priority
# 1. FREETYPE_INCLUDEPATH / FREETYPE_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

freetype {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
FREETYPE_DIR = $$(FREETYPEDIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(FREETYPE_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/freetype2) {
      FREETYPE_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/freetype2
      FREETYPE_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
}

isEmpty(FREETYPE_INCLUDEPATH) {
  FREETYPE_CFLAGS = $$system("pkg-config --cflags freetype2")
} else {
  FREETYPE_CFLAGS = -I$$FREETYPE_INCLUDEPATH
}

isEmpty(FREETYPE_LIBPATH) {
  FREETYPE_LIBS = $$system("pkg-config --libs freetype2")
} else {
  FREETYPE_LIBS = -L$$FREETYPE_LIBPATH -lfreetype
}

QMAKE_CXXFLAGS += $$FREETYPE_CFLAGS
LIBS += $$FREETYPE_LIBS
}
