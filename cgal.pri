cgal {
  DEFINES += ENABLE_CGAL

  # Optionally specify location of CGAL using the 
  # CGALDIR env. variable
  CGAL_DIR = $$(CGALDIR)
  !isEmpty(CGAL_DIR) {
    QMAKE_INCDIR += $$CGAL_DIR/include
    win*: QMAKE_INCDIR += $$CGAL_DIR/auxiliary/gmp/include
    QMAKE_LIBDIR += $$CGAL_DIR/lib
    message("CGAL location: $$CGAL_DIR")
  }

  QMAKE_CXXFLAGS += -std=c++11

  CONFIG(mingw-cross-env) {
    LIBS += -lgmp -lmpfr -lCGAL
    QMAKE_CXXFLAGS += -frounding-math 
  } else {
    win* {
      *-g++* { 
        QMAKE_CXXFLAGS += -frounding-math 
      }
      LIBS += $$CGAL_DIR/auxiliary/gmp/lib/libmpfr-4.lib -lCGAL-vc110-mt-gd
    } else {
      LIBS += -lgmp -lmpfr -lCGAL
      QMAKE_CXXFLAGS += -frounding-math 
    }
  }

	*clang* {
		QMAKE_CXXFLAGS -= -frounding-math
	}
}
