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
  LIBS += mingw-cross-env/lib/libCGAL.a
  QMAKE_CXXFLAGS += -fpermissive

  Release:DESTDIR = release
  Release:OBJECTS_DIR = release/objects
  Release:MOC_DIR = release/moc
  Release:RCC_DIR = release/rcc
  Release:UI_DIR = release/ui

  Debug:DESTDIR = debug
  Debug:OBJECTS_DIR = debug/objects
  Debug:MOC_DIR = debug/moc
  Debug:RCC_DIR = debug/rcc
  Debug:UI_DIR = debug/ui
}

