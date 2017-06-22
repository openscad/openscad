CONFIG += debug
CONFIG -= qt
debug: DEFINES += DEBUG

TEMPLATE = app

INCLUDEPATH += ../src ../src/libtess2/Include
DEPENDPATH += ../src

# Handle custom library location.
# Used when manually installing 3rd party libraries
isEmpty(OPENSCAD_LIBDIR) OPENSCAD_LIBDIR = $$(OPENSCAD_LIBRARIES)
macx:isEmpty(OPENSCAD_LIBDIR) {
  exists(/opt/local):exists(/usr/local/Cellar) {
    error("It seems you might have libraries in both /opt/local and /usr/local. Please specify which one to use with qmake OPENSCAD_LIBDIR=<prefix>")
  } else {
    exists(/opt/local) {
      #Default to MacPorts on Mac OS X
      message("Automatically searching for libraries in /opt/local. To override, use qmake OPENSCAD_LIBDIR=<prefix>")
      OPENSCAD_LIBDIR = /opt/local
    } else:exists(/usr/local/Cellar) {
      message("Automatically searching for libraries in /usr/local. To override, use qmake OPENSCAD_LIBDIR=<prefix>")
      OPENSCAD_LIBDIR = /usr/local
    }
  }
}
!isEmpty(OPENSCAD_LIBDIR) {
  QMAKE_INCDIR = $$OPENSCAD_LIBDIR/include
  QMAKE_LIBDIR = $$OPENSCAD_LIBDIR/lib
}

TARGET = decompose
mac {
  CONFIG -= app_bundle
}

macx {
  # Mac needs special care to link against the correct C++ library
  # We attempt to auto-detect it by inspecting Boost
  dirs = $${BOOSTDIR} $${QMAKE_LIBDIR}
  for(dir, dirs) {
    system(grep -q __112basic_string $${dir}/libboost_thread* >& /dev/null) {
      message("Detected libc++-linked boost in $${dir}")
      CONFIG += libc++
    }
  }

  libc++ {
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_LFLAGS += -stdlib=libc++
    QMAKE_OBJECTIVE_CFLAGS += -stdlib=libc++
    # libc++ on requires Mac OS X 10.7+
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
  }
}

*g++* {
  # See Dec 2011 OpenSCAD mailing list, re: CGAL/GCC bugs.
  QMAKE_CXXFLAGS *= -fno-strict-aliasing
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedefs # ignored before 4.8
  # use of 'auto'
  QMAKE_CXXFLAGS += -std=c++11
}

*clang* {
  # http://llvm.org/bugs/show_bug.cgi?id=9182
  QMAKE_CXXFLAGS_WARN_ON += -Wno-overloaded-virtual
  # disable enormous amount of warnings about CGAL / boost / etc
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
  QMAKE_CXXFLAGS_WARN_ON += -Wno-c++11-extensions
  # might want to actually turn this on once in a while
  QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare
}

# Application configuration
CONFIG += cgal
CONFIG += boost
CONFIG += eigen
CONFIG += gettext

mac: {
   LIBS += -framework OpenGL
} else {
   LIBS += -lGL
}


include(../common.pri)

HEADERS += ../src/cgal.h \
           ../src/cgalutils.h \
           ../src/linalg.h \
           ../src/grid.h \
           ../src/hash.h \
           ../src/polyset.h \
           ../src/polyset-utils.h \
           ../src/printutils.h \
           ../src/GeometryUtils.h \
           ../src/libtess2/Include/tesselator.h \
           ../src/libtess2/Source/bucketalloc.h \
           ../src/libtess2/Source/dict.h \
           ../src/libtess2/Source/geom.h \
           ../src/libtess2/Source/mesh.h \
           ../src/libtess2/Source/priorityq.h \
           ../src/libtess2/Source/sweep.h \
           ../src/libtess2/Source/tess.h

SOURCES += decompose.cpp \
           ../src/polygon2d.cc \
           ../src/polygon2d-CGAL.cc \
           ../src/CGAL_Nef_polyhedron.cc \
           ../src/cgalutils.cc \
           ../src/cgalutils-applyops.cc \
           ../src/cgalutils-tess.cc \
           ../src/cgalutils-polyhedron.cc \
           ../src/polyset.cc \
           ../src/polyset-gl.cc \
           ../src/GeometryUtils.cc \
           ../src/svg.cc \
           ../src/grid.cc \
           ../src/hash.cc \
           ../src/node.cc \
           ../src/export.cc \
           ../src/export_stl.cc \
           ../src/export_amf.cc \
           ../src/export_off.cc \
           ../src/export_dxf.cc \
           ../src/export_svg.cc \
           ../src/export_nef.cc \
           ../src/polyset-utils.cc \
           ../src/progress.cc \
           ../src/printutils.cc \
           ../src/libtess2/Source/bucketalloc.c \
           ../src/libtess2/Source/dict.c \
           ../src/libtess2/Source/geom.c \
           ../src/libtess2/Source/mesh.c \
           ../src/libtess2/Source/priorityq.c \
           ../src/libtess2/Source/sweep.c \
           ../src/libtess2/Source/tess.c
