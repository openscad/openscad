cgal {
  DEFINES += ENABLE_CGAL

  isEmpty(DEPLOYDIR) {
    # Optionally specify location of CGAL using the 
    # CGALDIR env. variable
    CGAL_DIR = $$(CGALDIR)
    !isEmpty(CGAL_DIR) {
      INCLUDEPATH += $$CGAL_DIR/include
      win32: INCLUDEPATH += $$CGAL_DIR/auxiliary/gmp/include
      LIBS += -L$$CGAL_DIR/lib
      message("CGAL location: $$CGAL_DIR")
    }
  }

  win32 {
    LIBS += $$CGAL_DIR/auxiliary/gmp/lib/libmpfr-4.lib -lCGAL-vc90-mt-s
  } else {
    LIBS += -lgmp -lmpfr -lCGAL
  }
  QMAKE_CXXFLAGS += -frounding-math
}
