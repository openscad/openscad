eigen2 {
  # Optionally specify location of Eigen2 using the 
  # EIGEN2DIR env. variable
  EIGEN2_DIR = $$(EIGEN2DIR)
  !isEmpty(EIGEN2_DIR) {
    EIGEN2_INCLUDEPATH = $$EIGEN2_DIR
  }
  else {
    CONFIG(mingw-cross-env) {
      EIGEN2_INCLUDEPATH = mingw-cross-env/include/eigen2
    } else {
      freebsd-g++: EIGEN2_INCLUDEPATH *= /usr/local/include/eigen2
      macx: EIGEN2_INCLUDEPATH *= /opt/local/include/eigen2
      !macx:!freebsd-g++:!win32:EIGEN2_INCLUDEPATH *= /usr/include/eigen2
    }
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
