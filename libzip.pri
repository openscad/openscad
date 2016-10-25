# Detect libzip, then use this priority list to determine
# which library to use:
#
# Priority
# 1. LIBZIP_INCLUDEPATH / LIBZIP_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

libzip {
# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(LIBZIP_INCLUDEPATH) {
    LIBZIP_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include
    LIBZIP_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
  }
}

exists($$LIBZIP_INCLUDEPATH/zip.h) {
  ENABLE_LIBZIP=yes
} else {
  LIBZIP_INCLUDEPATH =
  LIBZIP_LIBPATH =
}

isEmpty(LIBZIP_INCLUDEPATH) {
  LIBZIP_CFLAGS = $$system("pkg-config --cflags libzip")
  !isEmpty(LIBZIP_CFLAGS) {
    ENABLE_LIBZIP=yes
  }
} else {
  LIBZIP_CFLAGS = -I$$LIBZIP_INCLUDEPATH
}

isEmpty(LIBZIP_LIBPATH) {
  LIBZIP_LIBS = $$system("pkg-config --libs libzip")
} else {
  LIBZIP_LIBS = -L$$LIBZIP_LIBPATH -lzip
}

!isEmpty(ENABLE_LIBZIP) {
  DEFINES += ENABLE_LIBZIP
  QMAKE_CXXFLAGS += $$LIBZIP_CFLAGS
  LIBS += $$LIBZIP_LIBS
}

}
