# Optionally specify location of Eigen2 using the 
# EIGEN2DIR env. variable
EIGEN2_DIR = $$(EIGEN2DIR)
!isEmpty(EIGEN2_DIR) {
  INCLUDEPATH += $$EIGEN2_DIR
}
else {
  unix:freebsd-g++ {
    INCLUDEPATH += /usr/local/include/eigen2
  } else macx {
    INCLUDEPATH += /opt/local/include/eigen2
  }
  else {
    INCLUDEPATH += /usr/include/eigen2
  }
}
