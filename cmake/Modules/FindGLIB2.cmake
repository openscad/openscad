find_package(PkgConfig REQUIRED QUIET)

pkg_search_module(GLIB2 glib-2.0)
set(GLIB2_DEFINITIONS ${GLIB2_CFLAGS_OTHER})
set(GLIB2_LIBRARY_NAMES ${GLIB2_LIBRARIES})
set(GLIB2_LIBRARIES "")
foreach(GLIB2_LIB ${GLIB2_LIBRARY_NAMES})
  set(TMP TMP-NOTFOUND)
  find_library(TMP NAMES ${GLIB2_LIB}
               PATHS ${GLIB2_LIBRARY_DIRS}
               PATHS ${GLIB2_LIBDIR}
               NO_DEFAULT_PATH)
  if(TMP)
    list(APPEND GLIB2_LIBRARIES "${TMP}")
  endif()
endforeach()
