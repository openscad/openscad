# Detect gettext, then use this priority list to determine
# which library to use:
#
# Priority
# 1. GETTEXT_INCLUDEPATH / GETTEXT_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

gettext {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
GETTEXT_DIR = $$(GETTEXTDIR)

macx: {
  isEmpty(GETTEXT_INCLUDEPATH) {
    !isEmpty(OPENSCAD_LIBRARIES_DIR) {
      GETTEXT_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include
      GETTEXT_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
  !isEmpty(GETTEXT_INCLUDEPATH): GETTEXT_CXXFLAGS = -I$$GETTEXT_INCLUDEPATH
  !isEmpty(GETTEXT_LIBPATH): GETTEXT_LIBS = -L$$GETTEXT_LIBPATH 
  GETTEXT_LIBS += -lintl -liconv
}

QMAKE_CXXFLAGS += $$GETTEXT_CXXFLAGS
LIBS += $$GETTEXT_LIBS
}
