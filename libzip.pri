# Detect libzip, then use this priority list to determine
# which library to use:
#
# Priority
# 1. LIBZIP_INCLUDEPATH / LIBZIP_LIBPATH (qmake parameter, not checked it given on commandline)
# 3. system's standard include paths from pkg-config

libzip {

!exists($$LIBZIP_INCLUDEPATH/zip.h) {
  LIBZIP_INCLUDEPATH =
  LIBZIP_LIBPATH =
}

isEmpty(LIBZIP_INCLUDEPATH) {
  LIBZIP_CFLAGS = $$system("pkg-config --cflags libzip")
} else {
  LIBZIP_CFLAGS = -I$$LIBZIP_INCLUDEPATH
}

isEmpty(LIBZIP_LIBPATH) {
  LIBZIP_LIBS = $$system("pkg-config --libs libzip")
} else {
  LIBZIP_LIBS = -L$$LIBZIP_LIBPATH -lzip
}

QMAKE_CXXFLAGS += $$LIBZIP_CFLAGS
LIBS += $$LIBZIP_LIBS

}
