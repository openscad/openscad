# Detect harfbuzz, then use this priority list to determine
# which library to use:
#
# Priority
# 1. HARFBUZZ_INCLUDEPATH / HARFBUZZ_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

harfbuzz {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
HARFBUZZ_DIR = $$(HARFBUZZDIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(HARFBUZZ_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/harfbuzz) {
      HARFBUZZ_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/harfbuzz
      HARFBUZZ_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
}

isEmpty(HARFBUZZ_INCLUDEPATH) {
  HARFBUZZ_CFLAGS = $$system("pkg-config --cflags harfbuzz")
} else {
  HARFBUZZ_CFLAGS = -I$$HARFBUZZ_INCLUDEPATH
}

isEmpty(HARFBUZZ_LIBPATH) {
  HARFBUZZ_LIBS = $$system("pkg-config --libs harfbuzz")
} else {
  HARFBUZZ_LIBS = -L$$HARFBUZZ_LIBPATH -lharfbuzz
}

QMAKE_CXXFLAGS += $$HARFBUZZ_CFLAGS
LIBS += $$HARFBUZZ_LIBS
}
