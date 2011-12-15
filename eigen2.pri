eigen2 {
  # Optionally specify location of Eigen2 using the 
  # EIGEN2DIR env. variable
  EIGEN2_DIR = $$(EIGEN2DIR)
  !isEmpty(EIGEN2_DIR) {
    INCLUDEPATH += $$EIGEN2_DIR
  }
  else {
    CONFIG(mingw-cross-env) {
      INCLUDEPATH += mingw-cross-env/include/eigen2
    } else {
      freebsd-g++: INCLUDEPATH += /usr/local/include/eigen2
      macx: INCLUDEPATH += /opt/local/include/eigen2
      !macx:!freebsd-g++:INCLUDEPATH += /usr/include/eigen2
    }
  }

  # disable Eigen SIMD optimizations for non-Mac OSX
  !macx {
    !freebsd-g++ {
      QMAKE_CXXFLAGS += -DEIGEN_DONT_ALIGN
    }
  }
}
