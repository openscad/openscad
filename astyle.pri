# Detect astyle, then use this priority list to determine
# which library to use:
#
# Priority
# 1. ASTYLE_INCLUDE_DIR / ASTYLE_LIB_DIR
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. System installed library

astyle {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
ASTYLE_INCLUDEPATH = $$(ASTYLE_INCLUDE_DIR)
ASTYLE_LIBPATH = $$(ASTYLE_LIB_DIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(ASTYLE_INCLUDEPATH) {
    ASTYLE_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include
    ASTYLE_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
  }
}

isEmpty(ASTYLE_INCLUDEPATH) {
  exists(/usr/include/astyle.h) {
    ASTYLE_INCLUDEPATH=/usr/include
    ASTYLE_LIBPATH=/usr/lib
  }
  exists(/usr/local/include/astyle.h) {
    ASTYLE_INCLUDEPATH=/usr/local/include
    ASTYLE_LIBPATH=/usr/local/lib
  }
  exists(/opt/include/astyle.h) {
    ASTYLE_INCLUDEPATH=/opt/include
    ASTYLE_LIBPATH=/opt/lib
  }
}

isEmpty(ASTYLE_INCLUDEPATH) {
  message("Could not find astyle library (http://astyle.sourceforge.net/), code formatting disabled")
} else {
  ASTYLE_CFLAGS = -I$$ASTYLE_INCLUDEPATH
  ASTYLE_LIBS = -L$$ASTYLE_LIBPATH -lastyle

  HAVE_ASTYLE = 1
  DEFINES += HAVE_ASTYLE
  QMAKE_CXXFLAGS += $$ASTYLE_CFLAGS
  LIBS += $$ASTYLE_LIBS
}

}
