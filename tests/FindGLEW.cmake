#
# Try to find GLEW library and include path.
# Once done this will define
#
# GLEW_FOUND
# GLEW_INCLUDE_PATH
# GLEW_LIBRARY
# 

# a few lines of this file are based on the LGPL code found at 
# http://openlibraries.org/browser/trunk/FindGLEW.cmake?rev=1383 


IF (WIN32 AND MSVC)
	IF (WIN32_STATIC_BUILD) # passed from caller
		SET(GLEW_LIB_SEARCH_NAME glew32s.lib) # static, non-debug (Release)
	ELSE ()
		SET(GLEW_LIB_SEARCH_NAME glew32.lib) # other. untested with OpenSCAD
	ENDIF()
ELSE () # GCC
	SET(GLEW_LIB_SEARCH_NAME "libglew32s.a")
ENDIF ()

IF (WIN32)
	FIND_PATH( GLEW_INCLUDE_PATH GL/glew.h
		$ENV{PROGRAMFILES}/GLEW/include
		${PROJECT_SOURCE_DIR}/src/nvgl/glew/include
		DOC "The directory where GL/glew.h resides")
	FIND_LIBRARY( GLEW_LIBRARY
		NAMES ${GLEW_LIB_SEARCH_NAME}
		PATHS
		$ENV{PROGRAMFILES}/GLEW/lib
		${PROJECT_SOURCE_DIR}/src/nvgl/glew/bin
		${PROJECT_SOURCE_DIR}/src/nvgl/glew/lib
		DOC "The GLEW library")
ELSE (WIN32)
        message(STATUS "GLEW_DIR: " ${GLEW_DIR})
	FIND_PATH( GLEW_INCLUDE_PATH GL/glew.h
                HINTS ${GLEW_DIR}/include 
		PATHS /usr/include /usr/local/include /usr/pkg/include
                NO_DEFAULT_PATH
		DOC "The directory where GL/glew.h resides")
	FIND_LIBRARY( GLEW_LIBRARY
		NAMES GLEW glew
		HINTS ${GLEW_DIR}/lib 
		PATHS /usr/lib /usr/local/lib /usr/pkg/lib
                NO_DEFAULT_PATH
		DOC "The GLEW library")
ENDIF (WIN32)

IF (GLEW_INCLUDE_PATH)
	SET( GLEW_FOUND 1 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
        MESSAGE(STATUS "GLEW include found in " ${GLEW_INCLUDE_PATH} )
        MESSAGE(STATUS "GLEW library found in " ${GLEW_LIBRARY} )
ELSE (GLEW_INCLUDE_PATH)
	SET( GLEW_FOUND 0 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
ENDIF (GLEW_INCLUDE_PATH)
