message(STATUS "running openscad/tests/FindGLIB2.cmake ...")

include(PkgConfigTools)
save_pkg_config_env()

# GLIB2 requires pkg-config to build. 
# If we are did an OPENSCAD_LIBRARIES dependency build of glib2, we need to 
# tell pkg-config to look under OPENSCAD_LIBRARIES dir.
# Otherwise, we need to use the system's pkg-config to find system's glib2
if (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  if (EXISTS "$ENV{OPENSCAD_LIBRARIES}/include/glib-2.0/glib.h")
    message(STATUS "found glib.h under OPENSCAD_LIBRARIES.")
    message(STATUS "redirecting pkg-config to look under OPENSCAD_LIBRARIES")
    set(ENV{PKG_CONFIG_PATH} "$ENV{OPENSCAD_LIBRARIES}/lib/pkgconfig")
    set(ENV{PKG_CONFIG_LIBDIR} "$ENV{OPENSCAD_LIBRARIES}/lib/pkgconfig")
  else()
    message(STATUS "attempting to find system glib2")
  endif()
endif()

find_package(PkgConfig REQUIRED)

pkg_search_module(GLIB2 REQUIRED glib-2.0)
message(STATUS "GLIB2_VERSION ${GLIB2_VERSION}")
#message(STATUS "GLIB2_LIBRARIES ${GLIB2_LIBRARIES}")
message(STATUS "GLIB2_LIBRARY_DIRS ${GLIB2_LIBRARY_DIRS}")
#message(STATUS "GLIB2_LDFLAGS ${GLIB2_LDFLAGS}")
#message(STATUS "GLIB2_LDFLAGS_OTHER ${GLIB2_LDFLAGS_OTHER}")
#message(STATUS "GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS}")
message(STATUS "GLIB2_INCLUDE_DIRS:")
foreach(glib2incdir ${GLIB2_INCLUDE_DIRS})
  message(STATUS "  " ${glib2incdir})
endforeach()
#message(STATUS "GLIB2_CFLAGS ${GLIB2_CFLAGS}")
#message(STATUS "GLIB2_CFLAGS_OTHER ${GLIB2_CFLAGS_OTHER}")
message(STATUS "GLIB2_LIBDIR ${GLIB2_LIBDIR}")


set(GLIB2_DEFINITIONS ${GLIB2_CFLAGS_OTHER})
#message(STATUS "GLIB2_DEFINITIONS ${GLIB2_DEFINITIONS}")

set(GLIB2_LIBRARY_NAMES ${GLIB2_LIBRARIES})
set(GLIB2_LIBRARIES "")
foreach(GLIB2_LIB ${GLIB2_LIBRARY_NAMES})
#  message(STATUS "lib: ${GLIB2_LIB}")
  set(TMP TMP-NOTFOUND)
  find_library(TMP NAMES ${GLIB2_LIB}
               PATHS ${GLIB2_LIBRARY_DIRS}
               PATHS ${GLIB2_LIBDIR}
               NO_DEFAULT_PATH)
#  message(STATUS "TMP: ${TMP}")
  list(APPEND GLIB2_LIBRARIES "${TMP}")
endforeach()
message(STATUS "GLIB2_LIBRARIES:")
foreach(glib2libdir ${GLIB2_LIBRARIES})
  message(STATUS "  " ${glib2libdir})
endforeach()

restore_pkg_config_env()
