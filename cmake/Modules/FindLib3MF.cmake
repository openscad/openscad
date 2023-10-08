#
# Try to find 3MF library and include path.
# Once done this will define
#
# LIB3MF_FOUND
# LIB3MF_CFLAGS
# LIB3MF_LIBDIR
# LIB3MF_INCLUDE_DIRS
# LIB3MF_LIBRARIES
#

# Don't specify REQUIRED here in case pkg-config fails.
# We still fall back to the rest of detection code here.
# Travis CI Ubuntu Trusty environment has some issue with pkg-config
# not finding the version.
if (Lib3MF_FIND_QUIETLY)
  set(_quiet_arg QUIET)
endif()

find_package(PkgConfig REQUIRED ${_quiet_arg})
pkg_check_modules(PC_LIB3MF lib3MF ${_quiet_arg})

# Check for 2.0 version pc file if 1.0 was not found.
if (NOT PC_LIB3MF_FOUND)
  pkg_check_modules(PC_LIB3MF lib3mf ${_quiet_arg})
endif()

set(LIB3MF_VERSION ${PC_LIB3MF_VERSION})
set(LIB3MF_FOUND ${PC_LIB3MF_FOUND})

find_path(LIB3MF_INCLUDE_DIRS
    NAMES Model/COM/NMR_DLLInterfaces.h lib3mf_implicit.hpp
    HINTS $ENV{LIB3MF_INCLUDEDIR}
          ${PC_LIB3MF_INCLUDEDIR}
          ${PC_LIB3MF_INCLUDE_DIRS}
    PATH_SUFFIXES lib3mf Bindings/Cpp
)

find_library(LIB3MF_LIBRARIES
    NAMES 3mf 3MF
    HINTS $ENV{LIB3MF_LIBDIR}
          ${PC_LIB3MF_LIBDIR}
          ${PC_LIB3MF_LIBRARY_DIRS}
)

if("${LIB3MF_LIBRARIES}" STREQUAL "LIB3MF_LIBRARIES-NOTFOUND")
  set(LIB3MF_LIBRARIES "")
endif()

# default to uppercase for 1.0 library name
set(LIB3MF_LIB "3MF")
set(LIB3MF_API "API 1.x")

# some distribution packages are missing version information for 2.0
if ("${LIB3MF_VERSION}" STREQUAL "" AND LIB3MF_FOUND)
  if (EXISTS "/usr/include/lib3mf" AND EXISTS "/usr/include/lib3mf/lib3mf_implicit.hpp")
    set(LIB3MF_VERSION "2.0.0")
  endif()
endif()

if (LIB3MF_VERSION VERSION_EQUAL 2.0.0 OR LIB3MF_VERSION VERSION_GREATER 2.0.0)
  set(LIB3MF_API "API 2.x")
  set(LIB3MF_LIB "3mf")
  add_definitions(-DLIB3MF_API_2)
endif()

if (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  if (EXISTS "$ENV{OPENSCAD_LIBRARIES}/include/lib3mf/Model/COM/NMR_DLLInterfaces.h")
    message(STATUS "found lib3mf (NMR_DLLInterfaces.h) in OPENSCAD_LIBRARIES.")
    set(LIB3MF_INCLUDE_DIRS "$ENV{OPENSCAD_LIBRARIES}/include/lib3mf" "$ENV{OPENSCAD_LIBRARIES}/include/lib3mf/Model/COM")
    set(LIB3MF_LIBDIR "$ENV{OPENSCAD_LIBRARIES}/lib")
  endif()
endif()

if ("${LIB3MF_LIBRARIES}" STREQUAL "")
  if (EXISTS "/opt/include/lib3mf/Model/COM/NMR_DLLInterfaces.h")
    set(LIB3MF_INCLUDE_DIRS "/opt/include/lib3mf" "/opt/include/lib3mf/Model/COM")
    set(LIB3MF_LIBDIR "/opt/lib")
  else()
    if (EXISTS "/usr/local/include/lib3mf/Model/COM/NMR_DLLInterfaces.h")
      set(LIB3MF_INCLUDE_DIRS "/usr/local/include/lib3mf" "/usr/local/include/lib3mf/Model/COM")
      set(LIB3MF_LIBDIR "/usr/local/lib")
    else()
      if (EXISTS "/usr/include/lib3mf/Model/COM/NMR_DLLInterfaces.h")
        set(LIB3MF_INCLUDE_DIRS "/usr/include/lib3mf" "/usr/include/lib3mf/Model/COM")
        set(LIB3MF_LIBDIR "/usr/lib")
      endif()
    endif()
  endif()
endif()

if (NOT "${LIB3MF_LIBDIR}" STREQUAL "")
  set(LIB3MF_LIBRARIES "-L${LIB3MF_LIBDIR}" "-l${LIB3MF_LIB}")
endif()

if (NOT "${LIB3MF_LIBRARIES}" STREQUAL "")
  find_library(LIBZ_LIBRARY NAMES z)
  find_library(LIBZIP_LIBRARY NAMES zip)
  set(LIB3MF_LIBRARIES "${LIB3MF_LIBRARIES}" "${LIBZIP_LIBRARY}" "${LIBZ_LIBRARY}")
  set(LIB3MF_DEFINITIONS "__GCC" "ENABLE_LIB3MF")
  set(LIB3MF_FOUND TRUE)
else()
  set(LIB3MF_API "disabled")
endif()
