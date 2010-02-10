opencsg {
  HEADERS += src/render-opencsg.h
  SOURCES += src/render-opencsg.cc

  DEFINES += ENABLE_OPENCSG
  LIBS += -lopencsg
  unix:LIBS += -lGLEW
  win32:LIBS += -lglew32

  # Optionally specify location of OpenCSG using the 
  # OPENCSGDIR env. variable
  OPENCSG_DIR = $$(OPENCSGDIR)
  !isEmpty(OPENCSG_DIR) {
    INCLUDEPATH += $$OPENCSG_DIR/include
    LIBS += -L$$OPENCSG_DIR/lib
  }
  macx {
    # For glew
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib
  }
}
