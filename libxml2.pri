# Detect libxml2, then use this priority list to determine
# which library to use:
#
# Priority
# 1. LIBXML2_INCLUDEPATH / LIBXML2_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

libxml2 {
# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
LIBXML2_DIR = $$(LIBXML2DIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(LIBXML2_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/libxml2) {
      LIBXML2_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/libxml2
      LIBXML2_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
}

isEmpty(LIBXML2_INCLUDEPATH) {
  LIBXML2_CFLAGS = $$system("pkg-config --cflags libxml-2.0")
} else {
  LIBXML2_CFLAGS = -I$$LIBXML2_INCLUDEPATH
}

isEmpty(LIBXML2_LIBPATH) {
  LIBXML2_LIBS = $$system("pkg-config --libs libxml-2.0")
} else {
  LIBXML2_LIBS = -L$$LIBXML2_LIBPATH -lxml2
}

CONFIG(mingw-cross-env): {
  LIBXML2_LIBS += -llzma
  !CONFIG(mingw-cross-env-shared) {
    DEFINES += LIBXML_STATIC
  }
}

QMAKE_CXXFLAGS += $$LIBXML2_CFLAGS
LIBS += $$LIBXML2_LIBS
}
