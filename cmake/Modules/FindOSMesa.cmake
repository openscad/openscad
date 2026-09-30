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
  pkg_check_modules(PC_OSMESA QUIET osmesa)
endif()

# Hints for Homebrew installations (both linked and keg-only)
set(_OSMESA_BREW_PREFIX "")
if(APPLE AND NOT CMAKE_CROSSCOMPILING)
  find_program(_BREW_BIN brew)
  if(_BREW_BIN)
    execute_process(
      COMMAND ${_BREW_BIN} --prefix osmesa
      OUTPUT_VARIABLE _OSMESA_BREW_PREFIX
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
  endif()
endif()

find_path(OSMESA_INCLUDE_DIR
  NAMES GL/osmesa.h
  HINTS
    ${OSMesa_ROOT}
    ${OSMESA_ROOT}
    ${CMAKE_PREFIX_PATH}
    ${PC_OSMESA_INCLUDEDIR}
    ${PC_OSMESA_INCLUDE_DIRS}
    ${_OSMESA_BREW_PREFIX}
  PATH_SUFFIXES
    include
  PATHS
    /opt/homebrew/opt/osmesa/include
    /usr/local/opt/osmesa/include
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
    ${PC_OSMESA_LIBDIR}
    ${PC_OSMESA_LIBRARY_DIRS}
    ${_OSMESA_BREW_PREFIX}
  PATH_SUFFIXES
    lib
  PATHS
    /opt/homebrew/opt/osmesa/lib
    /usr/local/opt/osmesa/lib
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
