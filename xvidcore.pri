# Detect xvidcore, then use this priority list to determine
# which library to use:
#
# Priority
# 1. XVIDCORE_INCLUDEPATH / XVIDCORE_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

xvidcore {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
XVIDCORE_DIR = $$(XVIDCOREDIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(XVIDCORE_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/xvid.h) {
      XVIDCORE_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include
      XVIDCORE_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
}

!win*: {
  isEmpty(XVIDCORE_INCLUDEPATH) {
    exists(/usr/include/xvid.h) {
      XVIDCORE_INCLUDEPATH = /usr/include
      XVIDCORE_LIBPATH = /usr/lib
    }
    exists(/usr/local/include/xvid.h) {
      XVIDCORE_INCLUDEPATH = /usr/local/include
      XVIDCORE_LIBPATH = /usr/local/lib
    }
    exists(/opt/include/xvid.h) {
      XVIDCORE_INCLUDEPATH = /opt/include
      XVIDCORE_LIBPATH = /opt/lib
    }
  }
}

!isEmpty(XVIDCORE_INCLUDEPATH) {
  QMAKE_CXXFLAGS += -I$$XVIDCORE_INCLUDEPATH
  LIBS += -L$$XVIDCORE_LIBPATH -lxvidcore
  DEFINES += ENABLE_VIDEO_XVID

  HEADERS += src/video_xvid.h
  SOURCES += src/video_xvid.cc
}

}
