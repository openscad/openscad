opencsg {
  DEFINES += ENABLE_OPENCSG

  # Optionally specify location of OpenCSG using the 
  # OPENCSGDIR env. variable
  OPENCSG_DIR = $$(OPENCSGDIR)
  !isEmpty(OPENCSG_DIR) {
    QMAKE_INCDIR += $$OPENCSG_DIR/include
    QMAKE_LIBDIR += $$OPENCSG_DIR/lib
    message("OpenCSG location: $$OPENCSG_DIR")
  }

  LIBS += -lopencsg
}
