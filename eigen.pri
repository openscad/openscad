# Detect eigen3
#
# Priority
# 0. EIGENDIR if set
# 1. OPENSCAD_LIBRARIES eigen3
# 3. system's standard include paths for eigen3

eigen {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
EIGEN_DIR = $$(EIGENDIR)

# Optionally specify location of Eigen3 using the 
# OPENSCAD_LIBRARIES env. variable
!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(EIGEN_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/eigen3) {
      EIGEN_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/eigen3
    } 
  }
}

!isEmpty(EIGEN_DIR) { 
  EIGEN_INCLUDEPATH = $$EIGEN_DIR
  message("User set EIGEN location: $$EIGEN_INCLUDEPATH")
}

isEmpty(EIGEN_INCLUDEPATH) {
  linux*|hurd*|unix: EIGEN_INCLUDEPATH = /usr/include/eigen3
  freebsd-g++: EIGEN_INCLUDEPATH = /usr/local/include/eigen3
  netbsd*: EIGEN_INCLUDEPATH = /usr/pkg/include/eigen3
  macx: EIGEN_INCLUDEPATH = /opt/local/include/eigen3
}

!exists($$EIGEN_INCLUDEPATH/Eigen/Core) {
  EIGEN_CFLAGS = $$system("pkg-config --cflags eigen3")
  EIGEN_INCLUDEPATH = $$replace(EIGEN_CFLAGS,"-I","")
}

# disable Eigen SIMD optimizations for platforms where it breaks compilation
!macx {
  !freebsd-g++ {
    QMAKE_CXXFLAGS += -DEIGEN_DONT_ALIGN
  }
}

# EIGEN being under 'include/eigen3' needs special prepending
contains(QT_VERSION, ^5\\..*) {
  QMAKE_INCDIR = $$EIGEN_INCLUDEPATH $$QMAKE_INCDIR
} else {
  QMAKE_INCDIR_QT = $$EIGEN_INCLUDEPATH $$QMAKE_INCDIR_QT
}

# qmakespecs on netbsd prepend system includes, we need eigen first. 
netbsd* {
  QMAKE_CXXFLAGS = -I$$EIGEN_INCLUDEPATH $$QMAKE_CXXFLAGS
}

} # eigen
