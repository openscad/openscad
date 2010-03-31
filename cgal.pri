cgal {
  DEFINES += ENABLE_CGAL
  LIBS += -lCGAL

  # Optionally specify location of CGAL using the 
  # CGALDIR env. variable
  CGAL_DIR = $$(CGALDIR)
  !isEmpty(CGAL_DIR) {
    INCLUDEPATH += $$CGAL_DIR/include
    LIBS += -L$$CGAL_DIR/lib
    message("CGAL location: $$CGAL_DIR")
  }
  macx {
    # The -L/usr/lib is to force the linker to use system libraries over MacPort libraries
    LIBS += -L/usr/lib -L/opt/local/lib /opt/local/lib/libgmp.a /opt/local/lib/libmpfr.a /opt/local/lib/libboost_thread-mt.a
  }
  else {
    LIBS += -lmpfr
  }
  win32:LIBS += -lboost_thread -lgmp
  QMAKE_CXXFLAGS += -frounding-math
}
