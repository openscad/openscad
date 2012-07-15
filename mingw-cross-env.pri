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


	# Use different location for the cross-compiled binaries + .o files
	# This allows compiling unix build + mingw build in same tree

	DESTDIR=.

	Release:DESTDIR = release_mingw32
	Debug:DESTDIR = debug_mingw32

	OBJECTS_DIR = $$DESTDIR/objects
	MOC_DIR = $$DESTDIR/moc
	RCC_DIR = $$DESTDIR/rcc
	UI_DIR = $$DESTDIR/ui

}


