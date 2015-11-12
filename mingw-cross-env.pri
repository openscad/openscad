# cross compilation unix->win using the MXE system (mxe.cc)
# To use mostly static lib linking, pass CONFIG+=mingw-cross-env to qmake
# To use mostly shared lib DLL linking, pass CONFIG+=mingw-cross-env-shared

### Mostly Static link
# Some of these .a files are import libs for DLLs that exist on Windows systems
# Others are actual static .a files full of program code. 
# You can use use nm,objdump, or ar to determine the difference.
CONFIG(mingw-cross-env): {
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libglew32s.a 
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libglut.a 
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libopengl32.a 
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libGLEW.a 
#  exists( $$(MXE_SYS_DIR_STATIC)/lib/libglaux.a ) {
#    LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libglaux.a
#  }
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libglu32.a 
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libopencsg.a 
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libmpfr.a 
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libgmp.a 
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libCGAL.a
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libfontconfig.a
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libfreetype.a
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libharfbuzz.a
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libbz2.a
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libexpat.a
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libintl.a
  LIBS += $$(MXE_SYS_DIR_STATIC)/lib/libiconv.a
}

## Shared library link, to .dll files
CONFIG(mingw-cross-env-shared): {
  # on MXE, the shared library .dll files are under 'bin' not 'lib'.
  QMAKE_LFLAGS += -L./$$(MXE_SYS_DIR_SHARED)/bin
  LIBS += -lgmp -lmpfr -lCGAL 
  LIBS += -lopencsg -lglew32 -lglut -lopengl32 -lGLEW -lglu32
  LIBS += -lfontconfig -lfreetype -lharfbuzz -lbz2 -lexpat -lintl -liconv
}

CONFIG(mingw-cross-env)|CONFIG(mingw-cross-env-shared): {
  QMAKE_CXXFLAGS += -fpermissive
  WINSTACKSIZE = 8388608 # 8MB # github issue 116
  QMAKE_CXXFLAGS += -Wl,--stack,$$WINSTACKSIZE
  LIBS += -Wl,--stack,$$WINSTACKSIZE 
  QMAKE_DEL_FILE = rm -f
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedefs #eigen3
}
