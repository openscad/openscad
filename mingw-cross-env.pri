# cross compilation unix->win32
BUILDTYPE=$$(OPENSCAD_BUILDTYPE)
contains( BUILDTYPE, MXECROSS ) {
  message("mxe cross")
  LIBS += mingw-cross-env/lib/libglew32s.a 
  LIBS += mingw-cross-env/lib/libglut.a 
  LIBS += mingw-cross-env/lib/libopengl32.a 
  LIBS += mingw-cross-env/lib/libboost_thread_win32-mt.a
  LIBS += mingw-cross-env/lib/libboost_program_options-mt.a
  LIBS += mingw-cross-env/lib/libboost_filesystem-mt.a
  LIBS += mingw-cross-env/lib/libboost_system-mt.a
  LIBS += mingw-cross-env/lib/libboost_regex-mt.a
  LIBS += mingw-cross-env/lib/libboost_chrono-mt.a
  LIBS += mingw-cross-env/lib/libGLEW.a 
  LIBS += mingw-cross-env/lib/libglu32.a 
  LIBS += mingw-cross-env/lib/libopencsg.a 
  LIBS += mingw-cross-env/lib/libmpfr.a 
  LIBS += mingw-cross-env/lib/libgmp.a 
  LIBS += mingw-cross-env/lib/libCGAL.a
  LIBS += mingw-cross-env/lib/libfontconfig.a
  LIBS += mingw-cross-env/lib/libfreetype.a
  LIBS += mingw-cross-env/lib/libharfbuzz.a
  LIBS += mingw-cross-env/lib/libbz2.a
  LIBS += mingw-cross-env/lib/liblzma.a
  LIBS += mingw-cross-env/lib/libzip.a
  LIBS += mingw-cross-env/lib/libexpat.a
  LIBS += mingw-cross-env/lib/libintl.a
  LIBS += mingw-cross-env/lib/libiconv.a
  LIBS += mingw-cross-env/qt5/lib/libqscintilla2_qt5.a
  DEFINES += BOOST_THREAD_USE_LIB
  DEFINES += GLEW_STATIC
  DEFINES += LIBXML_STATIC
  QMAKE_CXXFLAGS += -fpermissive -frounding-math
  QMAKE_DEL_FILE = rm -f
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedefs #eigen3
}

CONFIG(mingw-cross-env-shared) {
  # on MXE, the shared library .dll files are under 'bin' not 'lib'.
  QMAKE_LFLAGS += -L./mingw-cross-env/bin
  LIBS += -lglew32 -lglut -lopengl32 -lGLEW -lglu32
  LIBS += -lopencsg -lmpfr -lgmp -lCGAL 
  LIBS += -lfontconfig -lfreetype -lharfbuzz -lbz2 -lexpat -lintl -liconv
  DEFINES += BOOST_STATIC
  DEFINES += Boost_USE_STATIC_LIBS
}
