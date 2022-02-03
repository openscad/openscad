find_path(SPNAV_INCLUDE_DIR NAMES spnav.h
	HINTS
	${PC_SPNAV_INCLUDEDIR}
	${PC_SPNAV_INCLUDE_DIRS})

find_library(SPNAV_LIBRARY NAMES spnav
	HINTS
	${PC_SPNAV_LIBDIR}
	${PC_SPNAV_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SpNav REQUIRED_VARS SPNAV_LIBRARY SPNAV_INCLUDE_DIR)

mark_as_advanced(SPNAV_INCLUDE_DIR SPNAV_LIBRARY)
