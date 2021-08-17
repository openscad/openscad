# - Try to find double-conversion
# Once done, this will define
#
# DoubleConversion_FOUND - system has double-conversion
# DoubleConversion_INCLUDE_DIRS - the double-conversion include directories
# DoubleConversion_LIBRARIES - link these to use double-conversion

include(FindPackageHandleStandardArgs)

find_library(DoubleConversion_LIBRARY double-conversion
  PATHS ${DoubleConversion_LIBRARYDIR})

find_path(DoubleConversion_INCLUDE_DIR double-conversion/double-conversion.h
  PATHS ${DoubleConversion_INCLUDEDIR})

find_package_handle_standard_args(DoubleConversion DEFAULT_MSG
  DoubleConversion_LIBRARY
  DoubleConversion_INCLUDE_DIR)

mark_as_advanced(
  DoubleConversion_LIBRARY
  DoubleConversion_INCLUDE_DIR)

if(DoubleConversion_FOUND)
  set(DoubleConversion_LIBRARIES ${DoubleConversion_LIBRARY})
  set(DoubleConversion_INCLUDE_DIRS ${DoubleConversion_INCLUDE_DIR})
endif()
