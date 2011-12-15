glew {
  isEmpty(OPENSCAD_LIBDIR) {
    # Optionally specify location of GLEW using the 
    # GLEWDIR env. variable
    GLEW_DIR = $$(GLEWDIR)
    isEmpty(GLEW_DIR) {
      # Default to MacPorts on Mac OS X
      macx: GLEW_DIR = /opt/local
    }
    !isEmpty(GLEW_DIR) {
      QMAKE_INCDIR += $$GLEW_DIR/include
      QMAKE_LIBDIR += -L$$GLEW_DIR/lib
      message("GLEW location: $$GLEW_DIR")
    }
  }

  unix:LIBS += -lGLEW
  win32:LIBS += -lglew32s
  CONFIG(mingw-cross-env):DEFINES += GLEW_STATIC
}
