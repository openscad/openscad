find_path(OSMESA_INCLUDE_DIR GL/osmesa.h)
find_library(OSMESA_LIBRARY NAMES OSMesa osmesa)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OSMesa
  DEFAULT_MSG
  OSMESA_LIBRARY
  OSMESA_INCLUDE_DIR
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

