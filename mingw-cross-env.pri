# cross compilation unix->win32
CONFIG(mingw-cross-env) {
  LIBS += mingw-cross-env/lib/libglew32s.a 
  LIBS += mingw-cross-env/lib/libglut.a 
  LIBS += mingw-cross-env/lib/libopengl32.a 
  LIBS += mingw-cross-env/lib/libGLEW.a 
  LIBS += mingw-cross-env/lib/libglaux.a 
  LIBS += mingw-cross-env/lib/libglu32.a 
  LIBS += mingw-cross-env/lib/libopencsg.a 
  LIBS += mingw-cross-env/lib/libmpfr.a 
  LIBS += mingw-cross-env/lib/libgmp.a 
  LIBS += mingw-cross-env/lib/libCGAL.a
  QMAKE_CXXFLAGS += -fpermissive
}


