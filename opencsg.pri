opencsg {
  DEFINES += ENABLE_OPENCSG
  CONFIG += glew
  include(glew.pri)

  HEADERS += src/OpenCSGRenderer.h
  SOURCES += src/OpenCSGRenderer.cc

  isEmpty(OPENSCAD_LIBDIR) {
    # Optionally specify location of OpenCSG using the 
    # OPENCSGDIR env. variable
    OPENCSG_DIR = $$(OPENCSGDIR)
    !isEmpty(OPENCSG_DIR) {
      QMAKE_INCDIR += $$OPENCSG_DIR/include
      QMAKE_LIBDIR += $$OPENCSG_DIR/lib
      message("OpenCSG location: $$OPENCSG_DIR")
    }
  }

  LIBS += -lopencsg
}
