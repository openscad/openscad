# Detect glib-2.0, then use this priority list to determine
# which library to use:
#
# Priority
# 1. GLIB2_INCLUDEPATH / GLIB2_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

glib-2.0 {
# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
GLIB2_DIR = $$(GLIB2DIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(GLIB2_INCLUDEPATH) {
    GLIB2_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/glib-2.0
    GLIB2_INCLUDEPATH_2 = $$OPENSCAD_LIBRARIES_DIR/lib/glib-2.0/include
    GLIB2_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
  }
}

!exists($$GLIB2_INCLUDEPATH/glib.h) {
  !exists($$GLIB2_INCLUDEPATH_2/glib.h) {
    GLIB2_INCLUDEPATH =
    GLIB2_INCLUDEPATH_2 =
    GLIB2_LIBPATH =
  }
}

isEmpty(GLIB2_INCLUDEPATH) {
  GLIB2_CFLAGS = $$system("pkg-config --cflags glib-2.0")
} else {
  GLIB2_CFLAGS = -I$$GLIB2_INCLUDEPATH
  GLIB2_CFLAGS += -I$$GLIB2_INCLUDEPATH_2
}

isEmpty(GLIB2_LIBPATH) {
  GLIB2_LIBS = $$system("pkg-config --libs glib-2.0")
} else {
  GLIB2_LIBS = -L$$GLIB2_LIBPATH -lglib-2.0
}

QMAKE_CXXFLAGS += $$GLIB2_CFLAGS
LIBS += $$GLIB2_LIBS
}

