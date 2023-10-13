# Try to find Nettle include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
# NETTLE_INCLUDE_DIRS - containing the nettle headers
# NETTLE_LIBRARIES - containing the nettle library

find_package(PkgConfig REQUIRED QUIET)

pkg_check_modules(PC_NETTLE nettle>=3.8) 

find_path(NETTLE_INCLUDE_DIRS NAMES nettle/sha2.h
	HINTS ${PC_NETTLE_INCLUDE_DIRS} ${PC_NETTLE_INCLUDEDIR}
)

find_library(NETTLE_LIBRARIES NAMES nettle
	HINTS ${PC_NETTLE_LIBRARY_DIRS} ${PC_NETTLE_LIBDIR}
)

set(NETTLE_VERSION ${PC_NETTLE_VERSION})
set(NETTLE_LIBRARIES ${PC_NETTLE_LIBRARIES})
set(NETTLE_INCLUDE_DIRS ${PC_NETTLE_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NETTLE DEFAULT_MSG NETTLE_INCLUDE_DIRS NETTLE_LIBRARIES)
