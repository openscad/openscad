# Detect potrace, then use this priority list to determine
# which library to use:
#
# Priority
# 1. POTRACE_INCLUDE_DIR / POTRACE_LIB_DIR
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. System installed library

potrace {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
POTRACE_INCLUDEPATH = $$(POTRACE_INCLUDE_DIR)
POTRACE_LIBPATH = $$(POTRACE_LIB_DIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(POTRACE_INCLUDEPATH) {
    POTRACE_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include
    POTRACE_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
  }
}

isEmpty(POTRACE_INCLUDEPATH) {
  exists(/usr/include/potracelib.h) {
    POTRACE_INCLUDEPATH=/usr/include
    POTRACE_LIBPATH=/usr/lib
  }
  exists(/usr/local/include/potracelib.h) {
    POTRACE_INCLUDEPATH=/usr/local/include
    POTRACE_LIBPATH=/usr/local/lib
  }
  exists(/opt/include/potracelib.h) {
    POTRACE_INCLUDEPATH=/opt/include
    POTRACE_LIBPATH=/opt/lib
  }
}

isEmpty(POTRACE_INCLUDEPATH) {
  message("Could not find potrace library (http://potrace.sourceforge.net/), trace() module disabled")
} else {
  POTRACE_CFLAGS = -I$$POTRACE_INCLUDEPATH
  POTRACE_LIBS = -L$$POTRACE_LIBPATH -lpotrace

  HAVE_POTRACE = 1
  DEFINES += HAVE_POTRACE
  QMAKE_CXXFLAGS += $$POTRACE_CFLAGS
  LIBS += $$POTRACE_LIBS
}

}
