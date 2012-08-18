eigen3 {
  CONFIG(mingw-cross-env) {
    EIGEN3_INCLUDEPATH = mingw-cross-env/include/eigen3
  }

  # Optionally specify location of Eigen3 using the 
  # OPENSCAD_LIBRARIES env. variable
  isEmpty(EIGEN3_INCLUDEPATH) {
    OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
    !isEmpty(OPENSCAD_LIBRARIES_DIR) {
      exists($$OPENSCAD_LIBRARIES_DIR/include/eigen3) {
        EIGEN3_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/eigen3
      }
    }
  }

  # Optionally specify location of Eigen3 using the 
  # EIGEN3DIR env. variable
  isEmpty(EIGEN3_INCLUDEPATH) {
    EIGEN3_DIR = $$(EIGEN3DIR)
    !isEmpty(EIGEN3_DIR) { 
      EIGEN3_INCLUDEPATH = $$EIGEN3_DIR
      message("EIGEN3 location: $$EIGEN3_INCLUDEPATH")
    }
  }

  isEmpty(EIGEN3_INCLUDEPATH) {
    freebsd-g++: EIGEN3_INCLUDEPATH = /usr/local/include/eigen3
    macx: EIGEN3_INCLUDEPATH = /opt/local/include/eigen3
    linux*|hurd*: EIGEN3_INCLUDEPATH = /usr/include/eigen3
    netbsd*: EIGEN3_INCLUDEPATH = /usr/pkg/include/eigen3
  }

  # EIGEN3 being under 'include/eigen3' needs special prepending
  QMAKE_INCDIR_QT = $$EIGEN3_INCLUDEPATH $$QMAKE_INCDIR_QT

  # disable Eigen SIMD optimizations for platforms where it breaks compilation
  !macx {
    !freebsd-g++ {
      QMAKE_CXXFLAGS += -DEIGEN_DONT_ALIGN
    }
  }
}
