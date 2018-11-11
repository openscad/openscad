#
# Try to find 3MF library and include path.
# Once done this will define
#
# LIB3MF_FOUND
# LIB3MF_CFLAGS
# LIB3MF_INCLUDE_DIRS
# LIB3MF_LDFLAGS
# 

message(STATUS "Searching for lib3mf.")

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
  set(LIB3MF_LDFLAGS "-L${LIB3MF_LIBDIR}" "-l3MF")
  set(LIB3MF_CFLAGS "-D__GCC -DENABLE_LIB3MF")
  message(STATUS "Found lib3mf in ${LIB3MF_LIBDIR}.")
else()
  message(STATUS "Could not find lib3mf.")
endif()
