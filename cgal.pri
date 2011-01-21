cgal {
  DEFINES += ENABLE_CGAL

  isEmpty(DEPLOYDIR) {
    # Optionally specify location of CGAL using the 
    # CGALDIR env. variable
    CGAL_DIR = $$(CGALDIR)
    !isEmpty(CGAL_DIR) {
      INCLUDEPATH += $$CGAL_DIR/include
      LIBS += -L$$CGAL_DIR/lib
      message("CGAL location: $$CGAL_DIR")
    }
  }

  LIBS += -lCGAL -lmpfr -lboost_thread
  win32 {
    LIBS += -lmpir
  } else {
    LIBS += -lgmp
  }
  QMAKE_CXXFLAGS += -frounding-math
}
