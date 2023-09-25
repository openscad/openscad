if (NOT $ENV{OPENCSGDIR} STREQUAL "")
  set(OPENCSG_DIR "$ENV{OPENCSGDIR}")
elseif (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  set(OPENCSG_DIR "$ENV{OPENSCAD_LIBRARIES}")
endif()
if (NOT OPENCSG_INCLUDE_DIR)
  message(STATUS "OPENCSG_DIR: " ${OPENCSG_DIR})
  find_path(OPENCSG_INCLUDE_DIR
            opencsg.h
            HINTS ${OPENCSG_DIR}/include)
  find_library(OPENCSG_LIBRARY
               opencsg
               HINTS ${OPENCSG_DIR}/lib)
  if (NOT OPENCSG_INCLUDE_DIR OR NOT OPENCSG_LIBRARY)
    message(FATAL_ERROR "OpenCSG not found")
  else()
    set(OPENCSG_FOUND TRUE)
    message(STATUS "OpenCSG include found in " ${OPENCSG_INCLUDE_DIR})
    message(STATUS "OpenCSG library found in " ${OPENCSG_LIBRARY})
  endif()
endif()

if (OPENCSG_FOUND)
  # version
  set(_VERSION_FILE ${OPENCSG_INCLUDE_DIR}/opencsg.h)
  if(EXISTS ${_VERSION_FILE})
    file(STRINGS ${_VERSION_FILE} _VERSION_LINE REGEX "define[ ]+OPENCSG_VERSION_STRING" )
    if(_VERSION_LINE)
      string (REGEX REPLACE ".*define[ ]+OPENCSG_VERSION_STRING[ ]+\".*\ (.*)\".*" "\\1" VERSION_STRING "${_VERSION_LINE}")
      set(OPENCSG_VERSION_STRING ${VERSION_STRING} CACHE STRING "OpenCSG version string")
    endif()
  endif()
endif()
