#
# Input:
# POTRACE_DIR root folder for potrace installation
#
# Output:
#  POTRACE_FOUND - System has potrace
#  POTRACE_INCLUDE_DIRS - The potrace include directories
#  POTRACE_LIBRARIES - The libraries needed to use potrace
#  POTRACE_DEFINITIONS - Compiler switches required for using potrace
#
find_path(POTRACE_INCLUDE_DIR NAMES potracelib.h PATHS $ENV{OPENSCAD_LIBRARIES}/include ${POTRACE_DIR}/include NO_DEFAULT_PATH)
find_path(POTRACE_INCLUDE_DIR potracelib.h)

find_library(POTRACE_LIBRARY NAMES potrace PATHS $ENV{OPENSCAD_LIBRARIES}/lib ${POTRACE_DIR}/lib NO_DEFAULT_PATH)
find_library(POTRACE_LIBRARY NAMES potrace)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(POTRACE DEFAULT_MSG POTRACE_LIBRARY POTRACE_INCLUDE_DIR)

set(POTRACE_INCLUDE_DIRS ${POTRACE_INCLUDE_DIR})
set(POTRACE_LIBRARIES ${POTRACE_LIBRARY})

message(STATUS "POTRACE include found in ${POTRACE_INCLUDE_DIRS}")
message(STATUS "POTRACE library found in ${POTRACE_LIBRARIES}")

mark_as_advanced(POTRACE_INCLUDE_DIR POTRACE_LIBRARY)