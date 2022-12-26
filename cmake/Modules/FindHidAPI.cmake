find_package(PkgConfig)
pkg_search_module(PC_HIDAPI QUIET hidapi hidapi-libusb)

find_path(HIDAPI_INCLUDE_DIR NAMES hidapi.h
	HINTS
	${PC_HIDAPI_INCLUDEDIR}
	${PC_HIDAPI_INCLUDE_DIRS})

find_library(HIDAPI_LIBRARY NAMES hidapi hidapi-libusb
	HINTS
	${PC_HIDAPI_LIBDIR}
	${PC_HIDAPI_LIBRARY_DIRS})

if(HIDAPI_INCLUDE_DIR AND EXISTS "${HIDAPI_INCLUDE_DIR}/hidapi.h")
  file(STRINGS "${HIDAPI_INCLUDE_DIR}/hidapi.h" hidapi_version_major
      REGEX "^#define[\t ]+HID_API_VERSION_MAJOR[\t ]+[0-9]+")
  file(STRINGS "${HIDAPI_INCLUDE_DIR}/hidapi.h" hidapi_version_minor
      REGEX "^#define[\t ]+HID_API_VERSION_MINOR[\t ]+[0-9]+")
  file(STRINGS "${HIDAPI_INCLUDE_DIR}/hidapi.h" hidapi_version_patch
      REGEX "^#define[\t ]+HID_API_VERSION_PATCH[\t ]+[0-9]+")
  string(REGEX REPLACE "[^0-9.]" "" HIDAPI_VERSION_STRING "${hidapi_version_major}.${hidapi_version_minor}.${hidapi_version_patch}")
  unset(hidapi_version_major)
  unset(hidapi_version_minor)
  unset(hidapi_version_patch)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HidAPI REQUIRED_VARS HIDAPI_LIBRARY HIDAPI_INCLUDE_DIR)

mark_as_advanced(HIDAPI_INCLUDE_DIR HIDAPI_LIBRARY)
