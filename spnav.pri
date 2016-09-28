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
    exists($$OPENSCAD_LIBRARIES_DIR/include/spnav.h) {
      SPNAV_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include
      SPNAV_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
}

isEmpty(SPNAV_INCLUDEPATH) {
  exists(/usr/include/spnav.h) {
    SPNAV_INCLUDEPATH = /usr/include
    SPNAV_LIBPATH = /usr/lib
  }
  exists(/usr/local/include/spnav.h) {
    SPNAV_INCLUDEPATH = /usr/local/include
    SPNAV_LIBPATH = /usr/local/lib
  }
  exists(/opt/include/spnav.h) {
    SPNAV_INCLUDEPATH = /opt/include
    SPNAV_LIBPATH = /opt/lib
  }
}

!isEmpty(SPNAV_INCLUDEPATH) {
  QMAKE_CXXFLAGS += -I$$SPNAV_INCLUDEPATH
  LIBS += -L$$SPNAV_LIBPATH -lspnav
  DEFINES += ENABLE_SPNAV

  HEADERS += src/input/SpaceNavInputDriver.h
  SOURCES += src/input/SpaceNavInputDriver.cc
}

}
