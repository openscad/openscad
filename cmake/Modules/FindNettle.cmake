# Try to find Nettle include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
# NETTLE_INCLUDE_DIRS - containing the nettle headers
# NETTLE_LIBRARIES - containing the nettle library

find_package(PkgConfig REQUIRED QUIET)

pkg_check_modules(PC_NETTLE nettle>=0.1) 

find_path(Nettle_INCLUDE_DIRS NAMES nettle/sha2.h
	HINTS ${PC_NETTLE_INCLUDE_DIRS} ${PC_NETTLE_INCLUDEDIR}
)

find_library(Nettle_LIBRARIES NAMES nettle libnettle
	HINTS ${PC_NETTLE_LIBRARY_DIRS} ${PC_NETTLE_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Nettle DEFAULT_MSG Nettle_INCLUDE_DIRS Nettle_LIBRARIES)
