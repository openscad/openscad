
macx {
	TARGET = OpenSCAD
	ICON = OpenSCAD.icns
}
else {
	TARGET = openscad
}

CONFIG += qt
TEMPLATE = app

CONFIG += debug
# CONFIG += release
# QMAKE_CFLAGS_RELEASE += -O3
# QMAKE_CXXFLAGS_RELEASE += -O3

# MDI needs an OpenCSG library that is compiled with OpenCSG-Reset-Hack.patch applied
# DEFINES += ENABLE_MDI

DEFINES += ENABLE_CGAL
LIBS += -lCGAL

macx {
	INCLUDEPATH += $(PWD)/../install/include $(PWD)/../OpenCSG-1.1.1/include /opt/local/include
	# The -L/usr/lib is to force the linker to use system libraries over MacPort libraries
	LIBS += -L/usr/lib -L$(PWD)/../install/lib -L$(PWD)/../OpenCSG-1.1.1/lib -L/opt/local/lib /opt/local/lib/libgmp.a /opt/local/lib/libmpfr.a /opt/local/lib/libboost_thread-mt.a
	QMAKE_CXXFLAGS += -frounding-math
}
else {
	LIBS += -lmpfr
}

DEFINES += ENABLE_OPENCSG
LIBS += -lopencsg

unix:LIBS += -lGLEW
win32:LIBS += -lglew32 -lboost_thread -lgmp

LEXSOURCES += lexer.l
YACCSOURCES += parser.y

FORMS   += MainWindow.ui

HEADERS += openscad.h \
           MainWindow.h \
           GLView.h \
           printutils.h

SOURCES += openscad.cc mainwin.cc glview.cc export.cc \
           value.cc expr.cc func.cc module.cc context.cc \
           csgterm.cc polyset.cc csgops.cc transform.cc \
           primitives.cc surface.cc control.cc render.cc \
           import.cc dxfdata.cc dxftess.cc dxfdim.cc \
           dxflinextrude.cc dxfrotextrude.cc highlighter.cc \
           printutils.cc

QMAKE_CXXFLAGS += -O0
# QMAKE_CXXFLAGS += -O3 -march=pentium

QT += opengl

target.path = /usr/local/bin/
INSTALLS += target

