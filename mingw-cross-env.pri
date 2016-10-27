<<<<<<< HEAD
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
=======
# cross compilation unix->win32
# depends on env variables set under scripts/setenv-mingw-xbuild.sh
CONFIG(mingw-cross-env) {
  LIBS += $$_MXE_TARGET_DIR/lib/libglew32s.a 
  LIBS += $$_MXE_TARGET_DIR/lib/libglut.a 
  LIBS += $$_MXE_TARGET_DIR/lib/libopengl32.a 
  LIBS += $$_MXE_TARGET_DIR/lib/libGLEW.a 
#  exists( $$_MXE_TARGET_DIR/lib/libglaux.a ) {
#    LIBS += $$_MXE_TARGET_DIR/lib/libglaux.a
#  }
  LIBS += $$_MXE_TARGET_DIR/lib/libglu32.a 
  LIBS += $$_MXE_TARGET_DIR/lib/libopencsg.a 
  LIBS += $$_MXE_TARGET_DIR/lib/libmpfr.a 
  LIBS += $$_MXE_TARGET_DIR/lib/libgmp.a 
  LIBS += $$_MXE_TARGET_DIR/lib/libCGAL.a
  LIBS += $$_MXE_TARGET_DIR/lib/libfontconfig.a
  LIBS += $$_MXE_TARGET_DIR/lib/libfreetype.a
  LIBS += $$_MXE_TARGET_DIR/lib/libharfbuzz.a
  LIBS += $$_MXE_TARGET_DIR/lib/libbz2.a
  LIBS += $$_MXE_TARGET_DIR/lib/libexpat.a
  LIBS += $$_MXE_TARGET_DIR/lib/libintl.a
  LIBS += $$_MXE_TARGET_DIR/lib/libiconv.a
  LIBS += $$_MXE_TARGET_DIR/lib/liblzma.a
>>>>>>> fbsdbuild
}

## Shared library link, to .dll files
CONFIG(mingw-cross-env-shared): {
  # on MXE, the shared library .dll files are under 'bin' not 'lib'.
<<<<<<< HEAD
  QMAKE_LFLAGS += -L./$$(MXE_SYS_DIR_SHARED)/bin
  LIBS += -lgmp -lmpfr -lCGAL 
  LIBS += -lopencsg -lglew32 -lglut -lopengl32 -lGLEW -lglu32
=======
  QMAKE_LFLAGS += -L./$$_MXE_TARGET_DIR/bin
  LIBS += -lglew32 -lglut -lopengl32 -lGLEW -lglu32
  LIBS += -lopencsg -lmpfr -lgmp -lCGAL 
>>>>>>> fbsdbuild
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
