# - Try to find Cairo
# Once done, this will define
#
#  CAIRO_FOUND - system has Cairo
#  CAIRO_INCLUDE_DIRS - the Cairo include directories
#  CAIRO_LIBRARIES - link these to use Cairo
#
# Copyright (C) 2012 Raphael Kubo da Costa <rakuco@webkit.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(PkgConfig)

if (APPLE)
  pkg_check_modules(PC_CAIRO QUIET cairo)

  find_path(CAIRO_INCLUDE_DIRS
    NAMES cairo.h
    HINTS ${PC_CAIRO_INCLUDEDIR}
          ${PC_CAIRO_INCLUDE_DIRS}
    PATH_SUFFIXES cairo
  )

  find_library(CAIRO_LIBRARIES
    NAMES cairo
    HINTS ${PC_CAIRO_LIBDIR}
          ${PC_CAIRO_LIBRARY_DIRS}
  )

  set(CAIRO_INCLUDEDIR "${CAIRO_INCLUDE_DIRS}")
elseif(MSVC)  
  pkg_check_modules(PC_CAIRO QUIET cairo)
  
  unset (_MSVC_INCLUDE_HINTS)
  unset (_MSVC_LIB_HINTS)
  
  list(APPEND _MSVC_INCLUDE_HINTS
      "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
      "${CMAKE_PREFIX_PATH}/include"
  )
  list(APPEND _MSVC_LIB_HINTS
      "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
      "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib"
      "${CMAKE_PREFIX_PATH}/lib"
  )
	
  find_path(CAIRO_INCLUDE_DIRS
    NAMES cairo.h
    HINTS ${PC_CAIRO_INCLUDEDIR}
          ${PC_CAIRO_INCLUDE_DIRS}
		  ${_MSVC_INCLUDE_HINTS}
    PATH_SUFFIXES cairo
  )
  find_library(CAIRO_LIBRARIES
    NAMES cairo
    HINTS ${PC_CAIRO_LIBDIR}
          ${PC_CAIRO_LIBRARY_DIRS}
		  ${_MSVC_LIB_HINTS}  
  )
  
  set(CAIRO_INCLUDEDIR "${CAIRO_INCLUDE_DIRS}")
else ()
  pkg_check_modules(CAIRO QUIET cairo)
endif ()

#message(STATUS "CAIRO_INCLUDEDIR: ${CAIRO_INCLUDEDIR}")

if (CAIRO_INCLUDEDIR)
    if (EXISTS "${CAIRO_INCLUDEDIR}/cairo-version.h")
        file(READ "${CAIRO_INCLUDEDIR}/cairo-version.h" CAIRO_VERSION_CONTENT)

        string(REGEX MATCH "#define +CAIRO_VERSION_MAJOR +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
        set(CAIRO_VERSION_MAJOR "${CMAKE_MATCH_1}")

        string(REGEX MATCH "#define +CAIRO_VERSION_MINOR +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
        set(CAIRO_VERSION_MINOR "${CMAKE_MATCH_1}")

        string(REGEX MATCH "#define +CAIRO_VERSION_MICRO +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
        set(CAIRO_VERSION_MICRO "${CMAKE_MATCH_1}")

        set(CAIRO_VERSION "${CAIRO_VERSION_MAJOR}.${CAIRO_VERSION_MINOR}.${CAIRO_VERSION_MICRO}")
    endif ()
endif ()

if ("${Cairo_FIND_VERSION}" VERSION_GREATER "${CAIRO_VERSION}")
    message(FATAL_ERROR "Required version (" ${Cairo_FIND_VERSION} ") is higher than found version (" ${CAIRO_VERSION} ")")
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Cairo
			REQUIRED_VARS  CAIRO_INCLUDE_DIRS CAIRO_LIBRARIES
			VERSION_VAR    CAIRO_VERSION
)

if(MSVC)
  if(CAIRO_FOUND AND NOT TARGET Cairo::Cairo)
	  
    add_library(Cairo::Cairo UNKNOWN IMPORTED)
    set_target_properties(Cairo::Cairo PROPERTIES
            IMPORTED_LOCATION             "${CAIRO_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${CAIRO_INCLUDE_DIRS}"
    )
	  	  
    #dependency for cario 
    find_package(Freetype QUIET)
    find_package(PNG QUIET)    
    find_package(ZLIB QUIET) 
    find_package(fontconfig QUIET)
	
    unset (_DEPS_FOR_CAIRO)
    #Freetype 	
	if(Freetype_FOUND OR FREETYPE_FOUND)
		if(NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype INTERFACE IMPORTED)
        set_target_properties(Freetype::Freetype PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${FREETYPE_LIBRARIES}"
        )
    endif()
		list(APPEND _DEPS_FOR_CAIRO Freetype::Freetype)
	endif()

    #Pixman
    pkg_check_modules(PIXMAN REQUIRED pixman-1)
    if (PIXMAN_FOUND AND NOT TARGET Pixman::Pixman)
      add_library(Pixman::Pixman INTERFACE IMPORTED)
      set_target_properties(Pixman::Pixman PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PIXMAN_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${PIXMAN_LIBRARIES}"
    )
    endif()

    #Zlib
    if (ZLIB_FOUND AND NOT TARGET ZLIB::ZLIB)
      add_library(ZLIB::ZLIB INTERFACE IMPORTED)
      set_target_properties(ZLIB::ZLIB PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIRS}"
          INTERFACE_LINK_LIBRARIES "${ZLIB_LIBRARIES}"
    )
    endif()

    #PNG 
    if (PNG_FOUND AND NOT TARGET PNG::PNG)
      add_library(PNG::PNG INTERFACE IMPORTED)
      set_target_properties(PNG::PNG PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${PNG_INCLUDE_DIRS}"
          INTERFACE_LINK_LIBRARIES "${PNG_LIBRARIES}"
    )
    endif()

    #Fontconfig
    pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
    if (FONTCONFIG_FOUND AND NOT TARGET Fontconfig::Fontconfig)
      add_library(Fontconfig::Fontconfig INTERFACE IMPORTED)
      set_target_properties(Fontconfig::Fontconfig PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${FONTCONFIG_INCLUDE_DIRS}"
          INTERFACE_LINK_LIBRARIES "${FONTCONFIG_LIBRARIES}"
    )
    endif()

    if(_DEPS_FOR_CAIRO)
        set_property(TARGET Cairo::Cairo APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_DEPS_FOR_CAIRO}")
    endif()

  endif()
endif()

mark_as_advanced(
    CAIRO_INCLUDE_DIRS
    CAIRO_LIBRARIES
)

#message(STATUS "CAIRO_LIBRARIES: ${CAIRO_LIBRARIES}")
#message(STATUS "CAIRO_INCLUDE_DIRS: ${CAIRO_INCLUDE_DIRS}")
#message(STATUS "_DEPS_FOR_CAIRO: ${_DEPS_FOR_CAIRO}")
#message(FATAL_ERROR "STOP HERE FOR DEBUG")
