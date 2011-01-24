cgal {
  DEFINES += ENABLE_CGAL

  isEmpty(DEPLOYDIR) {
    # Optionally specify location of CGAL using the 
    # CGALDIR env. variable
    CGAL_DIR = $$(CGALDIR)
    !isEmpty(CGAL_DIR) {
      INCLUDEPATH += $$CGAL_DIR/include
      win32 {
         LIBS += -L$$CGAL_DIR/build/lib -L$$(MPFRDIR)/build.vc10/lib/Win32/Release -L$$(MPIRDIR)
      } else {
         LIBS += -L$$CGAL_DIR/lib
      }
      message("CGAL location: $$CGAL_DIR")
    }
  }

  LIBS += -lmpfr
  win32 {
    LIBS += -lmpir -lCGAL-vc100-mt
  } else {
    LIBS += -lgmp -lCGAL
  }
  QMAKE_CXXFLAGS += -frounding-math
}
