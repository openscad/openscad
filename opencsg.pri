opencsg {
  DEFINES += ENABLE_OPENCSG
  CONFIG += glew
  include(glew.pri)

  HEADERS += src/OpenCSGRenderer.h
  SOURCES += src/OpenCSGRenderer.cc

  isEmpty(DEPLOYDIR) {
    # Optionally specify location of OpenCSG using the 
    # OPENCSGDIR env. variable
    OPENCSG_DIR = $$(OPENCSGDIR)
    !isEmpty(OPENCSG_DIR) {
      INCLUDEPATH += $$OPENCSG_DIR/include
      LIBS += -L$$OPENCSG_DIR/lib
      message("OpenCSG location: $$OPENCSG_DIR")
    }
  }

  LIBS += -lopencsg
}
