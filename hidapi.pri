# Detect hidapi, then use this priority list to determine
# which library to use:
#
# Priority
# 1. HIDAPI_INCLUDEPATH / HIDAPI_LIBPATH (qmake parameter, not checked it given on commandline)
# 2. OPENSCAD_LIBRARIES (environment variable)
# 3. system's standard include paths from pkg-config

hidapi {

# read environment variables
OPENSCAD_LIBRARIES_DIR = $$(OPENSCAD_LIBRARIES)
HIDAPI_DIR = $$(HIDAPIDIR)

!isEmpty(OPENSCAD_LIBRARIES_DIR) {
  isEmpty(HIDAPI_INCLUDEPATH) {
    exists($$OPENSCAD_LIBRARIES_DIR/include/hidapi) {
      HIDAPI_INCLUDEPATH = $$OPENSCAD_LIBRARIES_DIR/include/hidapi
      HIDAPI_LIBPATH = $$OPENSCAD_LIBRARIES_DIR/lib
    }
  }
}

!win*: {
  isEmpty(HIDAPI_INCLUDEPATH) {
    HIDAPI_CFLAGS = $$system("pkg-config --silence-errors --cflags hidapi-libusb")
  } else {
    HIDAPI_CFLAGS = -I$$HIDAPI_INCLUDEPATH
  }

  isEmpty(HIDAPI_LIBPATH) {
    HIDAPI_LIBS = $$system("pkg-config --silence-errors --libs hidapi-libusb")
  } else {
    HIDAPI_LIBS = -L$$HIDAPI_LIBPATH -lhidapi-libusb
  }
}

!isEmpty(HIDAPI_CFLAGS) {
  QMAKE_CXXFLAGS += $$HIDAPI_CFLAGS
  LIBS += $$HIDAPI_LIBS
  DEFINES += ENABLE_HIDAPI

  HEADERS += src/input/HidApiInputDriver.h
  SOURCES += src/input/HidApiInputDriver.cc
}

}
