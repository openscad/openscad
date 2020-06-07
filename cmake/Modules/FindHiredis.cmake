# FindHiredis.cmake - Try to find the Hiredis library

#  HIREDIS_FOUND - System has Hiredis
#  HIREDIS_INCLUDE_DIR - The Hiredis include directory
#  HIREDIS_LIBRARIES - The libraries needed to use Hiredis
#  HIREDIS_DEFINITIONS - Compiler switches required for using Hiredis

find_package(PkgConfig)
pkg_check_module(PC_HIREDIS QUITE hiredis)

find_path(HIREDIS_INCLUDE_DIRS
    NAMES hiredis.h
    HINTS
    ${PC_HIREDIS_INCLUDEDIR}
    ${PC_HIREDIS_INCLUDE_DIRS}
    PATH_SUFFIXES hiredis
)

find_library(HIREDIS_LIBRARIES
    NAMES hiredis
    HINTS
    ${PC_HIREDIS_DIR}
    ${PC_HIREDIS_LIBRARY_DIRS}
)


include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Hiredis DEFAULT_MSG HIREDIS_LIBRARIES HIREDIS_INCLUDE_DIR)

mark_as_advanced(
    HIREDIS_INCLUDE_DIR
    HIREDIS_LIBRARIES
)