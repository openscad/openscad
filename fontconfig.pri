# Detect fontconfig, then use this priority list to determine
# which library to use:
#
# Priority
# 1. FONTCONFIG_INCLUDEPATH / FONTCONFIG_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

fontconfig {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
FONTCONFIG_DIR = $$(FONTCONFIGDIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(FONTCONFIG_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/fontconfig) {
      FONTCONFIG_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/fontconfig
      FONTCONFIG_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
}

isEmpty(FONTCONFIG_INCLUDEPATH) {
  FONTCONFIG_CFLAGS = $$system("pkg-config --cflags fontconfig")
} else {
  FONTCONFIG_CFLAGS = -I$$FONTCONFIG_INCLUDEPATH
}

isEmpty(FONTCONFIG_LIBPATH) {
  FONTCONFIG_LIBS = $$system("pkg-config --libs fontconfig")
} else {
  FONTCONFIG_LIBS = -L$$FONTCONFIG_LIBPATH -lfontconfig
}

QMAKE_CXXFLAGS += $$FONTCONFIG_CFLAGS
LIBS += $$FONTCONFIG_LIBS
}
