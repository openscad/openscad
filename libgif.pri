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
}

!isEmpty(LIBGIF_INCLUDEPATH) {
  QMAKE_CXXFLAGS += -I$$LIBGIF_INCLUDEPATH
  LIBS += -L$$LIBGIF_LIBPATH -lgif
  DEFINES += ENABLE_VIDEO_GIF

#  HEADERS += src/video_gif.h
#  SOURCES += src/video_gif.cc
}

}
