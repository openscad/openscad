# cross compilation unix->win32
# To use static linking, pass CONFIG+=mingw-cross-env to qmake
# To use shared linking, pass CONFIG+=mingw-cross-env-shared to qmake
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
}

CONFIG(mingw-cross-env-shared) {
  # on MXE, the shared library .dll files are under 'bin' not 'lib'.
  QMAKE_LFLAGS += -L./mingw-cross-env/bin
  LIBS += -lglew32 -lglut -lopengl32 -lGLEW -lglu32
  LIBS += -lopencsg -lmpfr -lgmp -lCGAL 
  LIBS += -lfontconfig -lfreetype -lharfbuzz -lbz2 -lexpat -lintl -liconv
}

CONFIG(mingw-cross-env)|CONFIG(mingw-cross-env-shared) {
  QMAKE_CXXFLAGS += -fpermissive
  QMAKE_DEL_FILE = rm -f
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedefs #eigen3
}
