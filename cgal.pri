cgal {
# Uncomment this to enable experimental CGAL tesselation
# DEFINES += CGAL_TESSELATE
  DEFINES += ENABLE_CGAL
  LIBS += -lCGAL
  macx {
    INCLUDEPATH += $(PWD)/../install/include /opt/local/include
    # The -L/usr/lib is to force the linker to use system libraries over MacPort libraries
    LIBS += -L/usr/lib -L$(PWD)/../install/lib -L/opt/local/lib /opt/local/lib/libgmp.a /opt/local/lib/libmpfr.a /opt/local/lib/libboost_thread-mt.a
  }
  else {
    LIBS += -lmpfr
  }
  win32:LIBS += -lboost_thread -lgmp
  QMAKE_CXXFLAGS += -frounding-math
}
