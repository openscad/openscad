#[=======================================================================[.rst:
FindOSMesa
----------

Find the Off-Screen Mesa (OSMesa) library and headers.

Imported Targets
^^^^^^^^^^^^^^^^
``OSMesa::OSMesa``
  The OSMesa library, if found.

Result Variables
^^^^^^^^^^^^^^^^
``OSMESA_FOUND``
  True if OSMesa was found.
``OSMESA_INCLUDE_DIRS``
  Path to OSMesa include directories.
``OSMESA_LIBRARIES``
  Path to OSMesa libraries.
#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  set(_OSMESA_HINTS
    ${OSMesa_ROOT}
    ${OSMESA_ROOT}
    ${CMAKE_PREFIX_PATH}
    "${CMAKE_SOURCE_DIR}/dependencies/mesa_install"
    "${CMAKE_BINARY_DIR}/dependencies/mesa_install"
  )
  set(_OSMESA_SAVED_PKG_CONFIG_PATH "$ENV{PKG_CONFIG_PATH}")
  foreach(_hint ${_OSMESA_HINTS})
    if(EXISTS "${_hint}/lib/pkgconfig")
      set(ENV{PKG_CONFIG_PATH} "${_hint}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
  endforeach()
  pkg_check_modules(PC_OSMESA QUIET osmesa)
  set(ENV{PKG_CONFIG_PATH} "${_OSMESA_SAVED_PKG_CONFIG_PATH}")
endif()

find_path(OSMESA_INCLUDE_DIR
  NAMES GL/osmesa.h
  HINTS
    ${OSMesa_ROOT}
    ${OSMESA_ROOT}
    ${CMAKE_PREFIX_PATH}
    "${CMAKE_SOURCE_DIR}/dependencies/mesa_install"
    "${CMAKE_BINARY_DIR}/dependencies/mesa_install"
    ${PC_OSMESA_INCLUDEDIR}
    ${PC_OSMESA_INCLUDE_DIRS}
  PATH_SUFFIXES
    include
  PATHS
    /opt/homebrew/include
    /usr/local/include
    /usr/include
)

find_library(OSMESA_LIBRARY
  NAMES OSMesa osmesa OSMesa32
  HINTS
    ${OSMesa_ROOT}
    ${OSMESA_ROOT}
    ${CMAKE_PREFIX_PATH}
    "${CMAKE_SOURCE_DIR}/dependencies/mesa_install"
    "${CMAKE_BINARY_DIR}/dependencies/mesa_install"
    ${PC_OSMESA_LIBDIR}
    ${PC_OSMESA_LIBRARY_DIRS}
  PATH_SUFFIXES
    lib
  PATHS
    /opt/homebrew/lib
    /usr/local/lib
    /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OSMesa
  FOUND_VAR OSMESA_FOUND
  REQUIRED_VARS OSMESA_LIBRARY OSMESA_INCLUDE_DIR
  VERSION_VAR PC_OSMESA_VERSION
)

if(OSMESA_FOUND)
  set(OSMESA_INCLUDE_DIRS "${OSMESA_INCLUDE_DIR}")
  set(OSMESA_LIBRARIES "${OSMESA_LIBRARY}")

  if(NOT TARGET OSMesa::OSMesa)
    add_library(OSMesa::OSMesa UNKNOWN IMPORTED)
    set_target_properties(OSMesa::OSMesa PROPERTIES
      IMPORTED_LOCATION "${OSMESA_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${OSMESA_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(OSMESA_INCLUDE_DIR OSMESA_LIBRARY)
