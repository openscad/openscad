# cross compilation unix->win32
CONFIG(mingw-cross-env) {
  LIBS += mingw-cross-env/lib/libglew32s.a 
  LIBS += mingw-cross-env/lib/libglut.a 
  LIBS += mingw-cross-env/lib/libopengl32.a 
  LIBS += mingw-cross-env/lib/libGLEW.a 
#  exists( mingw-cross-env/lib/libglaux.a ) {
#    LIBS += mingw-cross-env/lib/libglaux.a
#  }
  LIBS += mingw-cross-env/lib/libglu32.a 
  LIBS += mingw-cross-env/lib/libopencsg.a 
  LIBS += mingw-cross-env/lib/libmpfr.a 
  LIBS += mingw-cross-env/lib/libgmp.a 
  LIBS += mingw-cross-env/lib/libCGAL.a
  LIBS += mingw-cross-env/lib/libfontconfig.a
  LIBS += mingw-cross-env/lib/libfreetype.a
  LIBS += mingw-cross-env/lib/libharfbuzz.a
  LIBS += mingw-cross-env/lib/libbz2.a
  LIBS += mingw-cross-env/lib/libexpat.a
  LIBS += mingw-cross-env/lib/libintl.a
  LIBS += mingw-cross-env/lib/libiconv.a
  QMAKE_CXXFLAGS += -fpermissive
  WINSTACKSIZE = 8388608 # 8MB # github issue 116
  QMAKE_CXXFLAGS += -Wl,--stack,$$WINSTACKSIZE
  LIBS += -Wl,--stack,$$WINSTACKSIZE 
  QMAKE_DEL_FILE = rm -f
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedefs #eigen3
}
