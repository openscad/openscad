# Detect libgif, then use this priority list to determine
# which library to use:
#
# Priority
# 1. LIBGIF_INCLUDEPATH / LIBGIF_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

libgif {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
LIBGIF_DIR = $$(LIBGIFDIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(LIBGIF_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/gif_lib.h) {
      LIBGIF_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include
      LIBGIF_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
}

!win*: {
  isEmpty(LIBGIF_INCLUDEPATH) {
    exists(/usr/include/gif_lib.h) {
      LIBGIF_INCLUDEPATH = /usr/include
      LIBGIF_LIBPATH = /usr/lib
    }
    exists(/usr/local/include/gif_lib.h) {
      LIBGIF_INCLUDEPATH = /usr/local/include
      LIBGIF_LIBPATH = /usr/local/lib
    }
    exists(/opt/include/gif_lib.h) {
      LIBGIF_INCLUDEPATH = /opt/include
      LIBGIF_LIBPATH = /opt/lib
    }
  }

  GIFLIB_MAJOR = $$system("grep GIFLIB_MAJOR $$LIBGIF_INCLUDEPATH/gif_lib.h | tr -d '[:alpha:][:punct:][:blank:]'")
  GIFLIB_MINOR = $$system("grep GIFLIB_MINOR $$LIBGIF_INCLUDEPATH/gif_lib.h | tr -d '[:alpha:][:punct:][:blank:]'")
  !lessThan(GIFLIB_MAJOR, 5) {
    greaterThan(GIFLIB_MAJOR, 5) {
      GIFLIB_VERSION_OK = true
    } else {
      !lessThan(GIFLIB_MINOR, 1) {
        GIFLIB_VERSION_OK = true
      }
    }
  }
}

!isEmpty(LIBGIF_INCLUDEPATH) {
  defined(GIFLIB_VERSION_OK, var) {
    message(Found libgif $${GIFLIB_MAJOR}.$${GIFLIB_MINOR})

    QMAKE_CXXFLAGS += -I$$LIBGIF_INCLUDEPATH
    LIBS += -L$$LIBGIF_LIBPATH -lgif
    DEFINES += ENABLE_VIDEO_GIF

    HEADERS += src/video/video_gif.h
    SOURCES += src/video/video_gif.cc
  } else {
    message(Found unsupported libgif version: $${GIFLIB_MAJOR}.$${GIFLIB_MINOR})
  }
}

}
