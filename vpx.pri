# Detect vpx, then use this priority list to determine
# which library to use:
#
# Priority
# 1. VPX_INCLUDEPATH / VPX_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

vpx {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
VPX_DIR = $$(VPXDIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(VPX_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/vpx) {
      VPX_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/vpx
      VPX_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
}

isEmpty(VPX_INCLUDEPATH) {
  VPX_CFLAGS = $$system("pkg-config --cflags vpx")
} else {
  VPX_CFLAGS = -I$$VPX_INCLUDEPATH
}

isEmpty(VPX_LIBPATH) {
  VPX_LIBS = $$system("pkg-config --libs vpx")
} else {
  VPX_LIBS = -L$$VPX_LIBPATH -lvpx
}

!isEmpty(VPX_LIBS) {
  QMAKE_CXXFLAGS += $$VPX_CFLAGS
  LIBS += $$VPX_LIBS
  DEFINES += ENABLE_VIDEO_VPX

  HEADERS += src/video/video_vpx.h
  SOURCES += src/video/video_vpx.cc
}

}
