# cross compilation unix->win32
# To use mostly static lib linking, pass CONFIG+=mingw-cross-env to qmake
# To use mostly shared lib DLL linking, pass CONFIG+=mingw-cross-env-shared

### Mostly Static link
# note... some of these .a files are actually "import libs" to allow 
# linking to Windows(TM) system DLLs at runtime without having access to 
# those DLLS at build time. Some other .a files are actual static libraries. 
# use nm/objdump/ar to determine the difference.
CONFIG(mingw-cross-env) {
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libglew32s.a 
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libglut.a 
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libopengl32.a 
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libGLEW.a 
#  exists( $$(MXE_TARGET_DIR_STATIC)/lib/libglaux.a ) {
#    LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libglaux.a
#  }
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libglu32.a 
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libopencsg.a 
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libmpfr.a 
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libgmp.a 
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libCGAL.a
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libfontconfig.a
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libfreetype.a
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libharfbuzz.a
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libbz2.a
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libexpat.a
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libintl.a
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libiconv.a
}

## Mostly shared library link, to .dll files
CONFIG(mingw-cross-env-shared) {
  # on MXE, the shared library .dll files are under 'bin' not 'lib'.
  QMAKE_LFLAGS += -L./$$(MXE_TARGET_DIR_SHARED)/bin
  # there is no .dll for mpfr due to unusual coding issues. link statically.
  LIBS += $$(MXE_TARGET_DIR_STATIC)/lib/libmpfr.a 
  LIBS += -lglew32 -lglut -lopengl32 -lGLEW -lglu32
  LIBS += -lopencsg -lgmp -lCGAL 
  LIBS += -lfontconfig -lfreetype -lharfbuzz -lbz2 -lexpat -lintl -liconv
}

CONFIG($$(MXE_TARGET_DIR))|CONFIG($$(MXE_TARGET_DIR)-shared) {
  QMAKE_CXXFLAGS += -fpermissive
  WINSTACKSIZE = 8388608 # 8MB # github issue 116
  QMAKE_CXXFLAGS += -Wl,--stack,$$WINSTACKSIZE
  LIBS += -Wl,--stack,$$WINSTACKSIZE 
  QMAKE_DEL_FILE = rm -f
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedefs #eigen3
}
