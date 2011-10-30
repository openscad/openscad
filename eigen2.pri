# Optionally specify location of Eigen2 using the 
# EIGEN2DIR env. variable
EIGEN2_DIR = $$(EIGEN2DIR)
!isEmpty(EIGEN2_DIR) {
  INCLUDEPATH += $$EIGEN2_DIR
}
else {
  macx {
    INCLUDEPATH += /opt/local/include/eigen2
  }
  else {
    CONFIG(mingw-cross-env) {
      INCLUDEPATH += mingw-cross-env/include/eigen2
    }
    else {
      INCLUDEPATH += /usr/include/eigen2
    }
  }
}
