cgal {
  DEFINES += ENABLE_CGAL

  !deploy {
    # Optionally specify location of CGAL using the 
    # CGALDIR env. variable
    CGAL_DIR = $$(CGALDIR)
    !isEmpty(CGAL_DIR) {
      INCLUDEPATH += $$CGAL_DIR/include
      LIBS += -L$$CGAL_DIR/lib
      message("CGAL location: $$CGAL_DIR")
    }
  }

  LIBS += -lCGAL -lmpfr -lgmp -lboost_thread
  QMAKE_CXXFLAGS += -frounding-math
}
