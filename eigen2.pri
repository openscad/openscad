eigen2 {

  CONFIG(mingw-cross-env) {
    EIGEN2_INCLUDEPATH = mingw-cross-env/include/eigen2
  }

  # Optionally specify location of Eigen2 using the 
  # OPENSCAD_LIBRARIES env. variable
  isEmpty(EIGEN2_INCLUDEPATH) {
    OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
    !isEmpty(OPENSCAD_LIBRARIES_DIR) {
      exists($$OPENSCAD_LIBRARIES_DIR/include/eigen2) {
        EIGEN2_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/eigen2
      }
    }
  }

  # Optionally specify location of Eigen2 using the 
  # EIGEN2DIR env. variable
  isEmpty(EIGEN2_INCLUDEPATH) {
    EIGEN2_DIR = $$(EIGEN2DIR)
    !isEmpty(EIGEN2_DIR) { 
      EIGEN2_INCLUDEPATH = $$EIGEN2_DIR
      message("EIGEN2 location: $$EIGEN2_INCLUDEPATH")
    }
  }

  isEmpty(EIGEN2_INCLUDEPATH) {
    freebsd-g++: EIGEN2_INCLUDEPATH = /usr/local/include/eigen2
    macx: EIGEN2_INCLUDEPATH = /opt/local/include/eigen2
    linux*: EIGEN2_INCLUDEPATH = /usr/include/eigen2
    netbsd*: EIGEN2_INCLUDEPATH = /usr/pkg/include/eigen2
  }

  # eigen2 being under 'include/eigen2' needs special prepending
  QMAKE_INCDIR_QT = $$EIGEN2_INCLUDEPATH $$QMAKE_INCDIR_QT

  # disable Eigen SIMD optimizations for platforms where it breaks compilation
  !macx {
    !freebsd-g++ {
      QMAKE_CXXFLAGS += -DEIGEN_DONT_ALIGN
    }
  }
}
