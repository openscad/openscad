# Windows console issues workaround stub.
#
# This attempts to solve the problem of piping OpenSCAD under windows
# command line (GUI mode programs in Windows dont allow this). We use
# the 'devenv' solution, which means building two binaries:
# openscad.exe, and openscad.com, the latter being a wrapper for the
# former. See src/winconsole.c for more details.
#
# Qmake doesn't like building two binaries in the same directory so we
# depend on release-common.sh to call qmake twice and package the file
# properly

TEMPLATE = app
TARGET = winconsole
FORMS =
HEADERS =
FLEXSOURCES =
BISONSOURCES =
RESOURCES =
SOURCES = winconsole.c
CONFIG -= qt
CONFIG += console # sets IMAGE_SUBSYSTEM_WINDOWS_CUI in binary
QMAKE_POST_LINK = cd $(DESTDIR) && mv winconsole.exe openscad.com
