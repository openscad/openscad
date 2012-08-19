# Detect eigen3 + eigen2, then use this priority list to determine
# which eigen to use:
#
# Priority
# 0. EIGENDIR if set (also EIGEN2DIR for backwards compatability)
# 1. OPENSCAD_LIBRARIES eigen3
# 2. OPENSCAD_LIBRARIES eigen2
# 3. system's standard include paths for eigen3
# 4. system's standard include paths for eigen2

eigen {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
EIGEN2_DIR = $$(EIGEN2DIR)
EIGEN_DIR = $$(EIGENDIR)

CONFIG(mingw-cross-env) {
  EIGEN_INCLUDEPATH = mingw-cross-env/include/eigen2
}

# Optionally specify location of Eigen3 using the 
# OPENSCAD_LIBRARIES env. variable
!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(EIGEN_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/eigen3) {
      EIGEN_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/eigen3
    } 
  }
  isEmpty(EIGEN_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/eigen2) {
      EIGEN_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/eigen2
    } 
  }
}


# Optionally specify location of Eigen using the 
# EIGENDIR env. variable (EIGEN2 for backwards compatability)
!isEmpty(EIGEN2_DIR) { 
  EIGEN_INCLUDEPATH = $$EIGEN2_DIR
  message("User set EIGEN location: $$EIGEN_INCLUDEPATH")
}
!isEmpty(EIGEN_DIR) { 
  EIGEN_INCLUDEPATH = $$EIGEN_DIR
  message("User set EIGEN location: $$EIGEN_INCLUDEPATH")
}

isEmpty(EIGEN_INCLUDEPATH) {
  freebsd-g++: EIGEN_INCLUDEPATH = /usr/local/include/eigen3
  macx: EIGEN_INCLUDEPATH = /opt/local/include/eigen3
  linux*|hurd*: EIGEN_INCLUDEPATH = /usr/include/eigen3
  netbsd*: EIGEN_INCLUDEPATH = /usr/pkg/include/eigen3
  !exists($$EIGEN_INCLUDEPATH) {
    freebsd-g++: EIGEN_INCLUDEPATH = /usr/local/include/eigen2
    macx: EIGEN_INCLUDEPATH = /opt/local/include/eigen2
    linux*|hurd*: EIGEN_INCLUDEPATH = /usr/include/eigen2
    netbsd*: EIGEN_INCLUDEPATH = /usr/pkg/include/eigen2
  }
}

# disable Eigen SIMD optimizations for platforms where it breaks compilation
!macx {
  !freebsd-g++ {
    QMAKE_CXXFLAGS += -DEIGEN_DONT_ALIGN
  }
}

# EIGEN being under 'include/eigen[2-3]' needs special prepending
QMAKE_INCDIR_QT = $$EIGEN_INCLUDEPATH $$QMAKE_INCDIR_QT

} # eigen
