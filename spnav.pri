# Detect spnav, then use this priority list to determine
# which library to use:
#
# Priority
# 1. SPNAV_INCLUDEPATH / SPNAV_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

spnav {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
SPNAV_DIR = $$(SPNAVDIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(SPNAV_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include) {
      SPNAV_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include
      SPNAV_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    } else {
      SPNAV_INCLUDEPATH = $$LIBRARIES_DIR/include
      SPNAV_LIBPATH = $$LIBRARIES_DIR/lib
    }
  }
}

isEmpty(SPNAV_INCLUDEPATH) {
  SPNAV_CFLAGS = -I/usr/include
}

isEmpty(SPNAV_LIBPATH) {
  SPNAV_LIBS = -L/usr/lib -lspnav
}

QMAKE_CXXFLAGS += $$SPNAV_CFLAGS
LIBS += $$SPNAV_LIBS
}
