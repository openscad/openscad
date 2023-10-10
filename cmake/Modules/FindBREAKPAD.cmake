###########################################################
#                  Find BREAKPAD Library
#----------------------------------------------------------
#
## 1: Setup:
# The following variables are optionally searched for defaults
#  BREAKPAD_DIR:            Base directory of OpenCv tree to use.
#
## 2: Variable
# The following are set after configuration is done: 
#  
#  BREAKPAD_FOUND
#  BREAKPAD_INCLUDE_DIRS
#  BREAKPAD_LIBS
#  BREAKPAD_DEFINITIONS
#
#----------------------------------------------------------

find_path(BREAKPAD_DIR "include/breakpad/client/linux/minidump_writer/minidump_writer.h"
    HINTS "${BREAKPAD_ROOT}" "$ENV{BREAKPAD_ROOT}" "$ENV{BREAKPAD_DIR}"
    PATHS "$ENV{PROGRAMFILES}" "$ENV{PROGRAMW6432}" "/usr" "/usr/local" "/usr/share" "/usr/local/share" "/usr/lib/cmake" "/usr/local/lib/cmake" "/usr/lib/x86_64-linux-gnu/cmake"
    PATH_SUFFIXES "BreakPad" "include"
    DOC "Root directory of BREAKPAD library")

##====================================================
## Find BREAKPAD libraries
##----------------------------------------------------
if(EXISTS "${BREAKPAD_DIR}")

	## Initiate the variable before the loop
	set(BREAKPAD_LIBS "${BREAKPAD_DIR}/lib/libbreakpad_client.a")
	set(BREAKPAD_FOUND TRUE)

	## Set all BREAKPAD component and their account into variables
	set(BREAKPAD_LIB_COMPONENTS common crash_generation_client crash_generation_server exception_handler)

	## Loop over each internal component and find its library file
	foreach(LIBCOMP ${BREAKPAD_LIB_COMPONENTS})

		find_library(BREAKPAD_${LIBCOMP}_LIBRARY_DEBUG NAMES "${LIBCOMP}" PATHS "${BREAKPAD_DIR}/lib${PACKAGE_LIB_SUFFIX_DBG}" NO_DEFAULT_PATH)
		find_library(BREAKPAD_${LIBCOMP}_LIBRARY_RELEASE NAMES "${LIBCOMP}" PATHS "${BREAKPAD_DIR}/lib${PACKAGE_LIB_SUFFIX_REL}" NO_DEFAULT_PATH)
		find_library(BREAKPAD_${LIBCOMP}_LIBRARY_ALL NAMES "${LIBCOMP}" PATH_SUFFIXES "breakpad")
		
		#Remove the cache value
		set(BREAKPAD_${LIBCOMP}_LIBRARY "" CACHE STRING "" FORCE)
		
		#both debug/release
		if(BREAKPAD_${LIBCOMP}_LIBRARY_DEBUG AND BREAKPAD_${LIBCOMP}_LIBRARY_RELEASE)
			set(BREAKPAD_${LIBCOMP}_LIBRARY debug ${BREAKPAD_${LIBCOMP}_LIBRARY_DEBUG} optimized ${BREAKPAD_${LIBCOMP}_LIBRARY_RELEASE} CACHE STRING "" FORCE)
		#only debug
		elseif(BREAKPAD_${LIBCOMP}_LIBRARY_DEBUG)
			set(BREAKPAD_${LIBCOMP}_LIBRARY ${BREAKPAD_${LIBCOMP}_LIBRARY_DEBUG} CACHE STRING "" FORCE)
		#only release
		elseif(BREAKPAD_${LIBCOMP}_LIBRARY_RELEASE)
			set(BREAKPAD_${LIBCOMP}_LIBRARY ${BREAKPAD_${LIBCOMP}_LIBRARY_RELEASE} CACHE STRING "" FORCE)
		#both debug/release
		elseif(BREAKPAD_${LIBCOMP}_LIBRARY_ALL)
			set(BREAKPAD_${LIBCOMP}_LIBRARY ${BREAKPAD_${LIBCOMP}_LIBRARY_ALL} CACHE STRING "" FORCE)
		#no library found
		else()
			message("BREAKPAD component NOT found: ${LIBCOMP}")
			set(BREAKPAD_FOUND FALSE)
		endif()
		
		#Add to the general list
		if(BREAKPAD_${LIBCOMP}_LIBRARY)
			set(BREAKPAD_LIBS ${BREAKPAD_LIBS} ${BREAKPAD_${LIBCOMP}_LIBRARY})
		endif()
			
	endforeach()

	set(BREAKPAD_INCLUDE_DIRS "${BREAKPAD_DIR}/include/breakpad" )
	message(STATUS "BREAKPAD found (include: ${BREAKPAD_INCLUDE_DIRS})")

else()

	package_report_not_found(BREAKPAD "Please specify BREAKPAD directory using BREAKPAD_ROOT env. variable")

endif()
##====================================================


##====================================================
if(BREAKPAD_FOUND)
	set(BREAKPAD_DIR "${BREAKPAD_DIR}" CACHE PATH "" FORCE)
	mark_as_advanced(BREAKPAD_DIR)
endif()
##====================================================
