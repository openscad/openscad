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

message(STATUS "Searching for lib3mf.")

# Don't specify REQUIRED here in case pkg-config fails.
# We still fall back to the rest of detection code here.
# Travis CI Ubuntu Trusty environment has some issue with pkg-config
# not finding the version.
pkg_check_modules(LIB3MF lib3MF)

# default to uppercase for 1.0 library name
set(LIB3MF_LIB "3MF")

# some distribution packages are missing version information for 2.0
if (LIB3MF_VERSION STREQUAL "" AND LIB3MF_FOUND)
  if (EXISTS "/usr/include/lib3mf" AND EXISTS "/usr/include/lib3mf/lib3mf_implicit.hpp")
    set(LIB3MF_VERSION "2.0.0")
  endif()
endif()

if (LIB3MF_VERSION VERSION_GREATER_EQUAL 1.8.1)
  message("lib3MF ${LIB3MF_VERSION} found: ${LIB3MF_INCLUDE_DIRS}")
endif()

if (LIB3MF_VERSION VERSION_GREATER_EQUAL 2.0.0)
  set(LIB3MF_API_2)
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

if ("${LIB3MF_LIBDIR}" STREQUAL "")
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

if (NOT ${LIB3MF_LIBDIR} STREQUAL "")
  set(LIB3MF_LIBRARIES "-L${LIB3MF_LIBDIR}" "-l${LIB3MF_LIB} -lzip -lz")
  set(LIB3MF_CFLAGS "-D__GCC -DENABLE_LIB3MF")
  set(LIB3MF_FOUND TRUE)
  message(STATUS "Found lib3mf in ${LIB3MF_LIBDIR}.")
else()
  message(STATUS "Could not find lib3mf.")
endif()
